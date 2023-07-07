#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "semant.h"
#include "utilities.h"

#include <map>
#include <set>
#include <vector>
#include <memory>

extern int semant_debug;
extern char *curr_filename;

//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
static Symbol 
    arg,
    arg2,
    Bool,
    concat,
    cool_abort,
    copy,
    Int,
    in_int,
    in_string,
    IO,
    length,
    Main,
    main_meth,
    No_class,
    No_type,
    Object,
    out_int,
    out_string,
    prim_slot,
    self,
    SELF_TYPE,
    Str,
    str_field,
    substr,
    type_name,
    val;
//
// Initializing the predefined symbols.
//
static void initialize_constants(void)
{
    arg         = idtable.add_string("arg");
    arg2        = idtable.add_string("arg2");
    Bool        = idtable.add_string("Bool");
    concat      = idtable.add_string("concat");
    cool_abort  = idtable.add_string("abort");
    copy        = idtable.add_string("copy");
    Int         = idtable.add_string("Int");
    in_int      = idtable.add_string("in_int");
    in_string   = idtable.add_string("in_string");
    IO          = idtable.add_string("IO");
    length      = idtable.add_string("length");
    Main        = idtable.add_string("Main");
    main_meth   = idtable.add_string("main");
    //   _no_class is a symbol that can't be the name of any 
    //   user-defined class.
    No_class    = idtable.add_string("_no_class");
    No_type     = idtable.add_string("_no_type");
    Object      = idtable.add_string("Object");
    out_int     = idtable.add_string("out_int");
    out_string  = idtable.add_string("out_string");
    prim_slot   = idtable.add_string("_prim_slot");
    self        = idtable.add_string("self");
    SELF_TYPE   = idtable.add_string("SELF_TYPE");
    Str         = idtable.add_string("String");
    str_field   = idtable.add_string("_str_field");
    substr      = idtable.add_string("substr");
    type_name   = idtable.add_string("type_name");
    val         = idtable.add_string("_val");
}

bool InheritanceNode::AddChild(InheritanceNode* newChild, std::string& errorString)
{
    auto insertionPair = m_children.insert(newChild);
    if (insertionPair.second == false) {
        // child class is multiply defined

        std::stringstream stream;
        stream << "Class " << newChild->m_name << " is multiply defined" << endl;
        errorString = stream.str();
        return false;
    }

    m_numDescendants += newChild->m_numDescendants + 1;
    newChild->m_parent = this;

    std::set<std::string> visitedParents;
    visitedParents.insert(m_name);

    InheritanceNode* parent = m_parent;
    while (parent != nullptr)
    {
        auto insertResult = visitedParents.insert(parent->m_name);
        if (insertResult.second == false)
        {
            // Cycle detected
            std::stringstream stream;
            stream << "Cycle detected with class " << newChild->m_name << " see ";
            errorString = stream.str();
            return false;
        }

        parent->m_numDescendants += newChild->m_numDescendants + 1;
        parent = parent->m_parent;
    }

    return true;
}

// todo: Might want to get rid of this copy to m_classes
ClassTable::ClassTable(Classes classes) : semant_errors(0) , error_stream(cerr), m_classes(classes) {
    install_basic_classes();

    using namespace std;

    map<string, unique_ptr<InheritanceNode>> inheritanceNodeMap;
    
    // Just used to check for multiply defined children
    set<string> allDefinedChildren;

    for(int i = m_classes->first(); m_classes->more(i); i = m_classes->next(i))
    { 
        string parentName = m_classes->nth(i)->get_parent()->get_string();
        string childName = m_classes->nth(i)->get_name()->get_string();

        if (parentName == Int->get_string() || parentName == Bool->get_string() || parentName == Str->get_string())
        {
            // class inherits from a basic type
            semant_error(m_classes->nth(i));
            error_stream << "Class " << childName << " inherits from either Int, Bool, or String. This is illegal." << endl;
            continue;
        }

        auto insertResult = allDefinedChildren.insert(childName);
        if (insertResult.second == false)
        {
            // class is defined multiple times
            semant_error(m_classes->nth(i));
            error_stream << "Class " << childName << " multiply defined" << endl;
            continue;
        }

        if (parentName == childName)
        {
            // class inheritis from itself 
            semant_error(m_classes->nth(i));
            error_stream << "Class " << childName << " inherits from itself" << endl;
            continue;
        }

        // first check to see if the child already exists
        if (inheritanceNodeMap.find(childName) != inheritanceNodeMap.end())
        {
            // Don't need to do anything here if the child already exists
        } 
        else
        {
            // create a new child node and add it to the map
            inheritanceNodeMap[childName] = make_unique<InheritanceNode>(childName);
        }

        // then find out if we have created the parent node already
        if (inheritanceNodeMap.find(parentName) != inheritanceNodeMap.end())
        {
            // The node already exists, insert child into its child set
            string error_msg;
            bool successfulInsertion = inheritanceNodeMap[parentName]->AddChild(inheritanceNodeMap[childName].get(), error_msg);
            if (successfulInsertion == false)
            {
                semant_error(m_classes->nth(i));
                error_stream << error_msg << endl;
                break;
            };
        }
        else
        {
            // add it to the map
            inheritanceNodeMap[parentName] = make_unique<InheritanceNode>(parentName);

            // insert child into its child set
            string error_msg;
            bool successfulInsertion = inheritanceNodeMap[parentName]->AddChild(inheritanceNodeMap[childName].get(), error_msg);
            if (successfulInsertion == false)
            {
                semant_error(m_classes->nth(i));
                error_stream << error_msg << endl;
                break;
            };
        }
    }

    // Every node must have a non-null parent (even object which has a no_class parent)
    for ( auto i = inheritanceNodeMap.begin(); i != inheritanceNodeMap.end(); ++i)
    {
        InheritanceNode* node = i->second.get();

        if (node->GetName() == No_class->get_string()) continue; //No class is the only exception to this rule

        if (node->HasParent() == false)
        {
            // Find the child class for this orphaned parent
            bool found = false; // just for sanity check, we should always be able to find the child of the undefined parent
            for(int i = m_classes->first(); m_classes->more(i); i = m_classes->next(i))
            { 
                if (node->GetName() == m_classes->nth(i)->get_parent()->get_string())
                {
                    found = true;
                    semant_error(m_classes->nth(i));
                    error_stream << "parent class of " << m_classes->nth(i)->get_name() << " is not defined" << endl;
                    break;
                }
            }
            if (found == false)
            {
                error_stream << "Programmer error! some assumption is wrong" << endl;
            }
        }
    }

    // Main must exist
    if (inheritanceNodeMap.find("Main") == inheritanceNodeMap.end())
    {
        semant_error();
        error_stream << "Main class is not defined in the program" << endl;
    }

    if (semant_debug)
    {
        cout << "The number of nodes in the inheritance node map is " << inheritanceNodeMap[No_class->get_string()]->GetNumDescendants() + 1 << endl;
        cout << "The number of classes that we encountred (parent and child) in the ast is " << inheritanceNodeMap.size() << endl;

        for(int i = m_classes->first(); m_classes->more(i); i = m_classes->next(i))
        { 
            cout << "Parent is:" << endl;
            cout << m_classes->nth(i)->get_parent()->get_string() << endl;
            cout << "Child is:" << endl;
            cout << m_classes->nth(i)->get_name()->get_string() << endl;
            cout << endl;
        }
    }
}

void ClassTable::install_basic_classes() {

    // The tree package uses these globals to annotate the classes built below.
   // curr_lineno  = 0;
    Symbol filename = stringtable.add_string("<basic class>");
    
    // The following demonstrates how to create dummy parse trees to
    // refer to basic Cool classes.  There's no need for method
    // bodies -- these are already built into the runtime system.
    
    // IMPORTANT: The results of the following expressions are
    // stored in local variables.  You will want to do something
    // with those variables at the end of this method to make this
    // code meaningful.

    // 
    // The Object class has no parent class. Its methods are
    //        abort() : Object    aborts the program
    //        type_name() : Str   returns a string representation of class name
    //        copy() : SELF_TYPE  returns a copy of the object
    //
    // There is no need for method bodies in the basic classes---these
    // are already built in to the runtime system.

    // todo: really unfortunate that there is no in place method to append these classes.. as far as I can tell it is copying the whole list to a new list with the appended node

    Class_ Object_class =
	class_(Object, 
	       No_class,
	       append_Features(
			       append_Features(
					       single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
					       single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
			       single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
	       filename);
    m_classes = append_Classes(m_classes, single_Classes(Object_class));

    // 
    // The IO class inherits from Object. Its methods are
    //        out_string(Str) : SELF_TYPE       writes a string to the output
    //        out_int(Int) : SELF_TYPE            "    an int    "  "     "
    //        in_string() : Str                 reads a string from the input
    //        in_int() : Int                      "   an int     "  "     "
    //
    Class_ IO_class = 
	class_(IO, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       single_Features(method(out_string, single_Formals(formal(arg, Str)),
										      SELF_TYPE, no_expr())),
							       single_Features(method(out_int, single_Formals(formal(arg, Int)),
										      SELF_TYPE, no_expr()))),
					       single_Features(method(in_string, nil_Formals(), Str, no_expr()))),
			       single_Features(method(in_int, nil_Formals(), Int, no_expr()))),
	       filename);
    m_classes = append_Classes(m_classes, single_Classes(IO_class));

    //
    // The Int class has no methods and only a single attribute, the
    // "val" for the integer. 
    //
    Class_ Int_class =
	class_(Int, 
	       Object,
	       single_Features(attr(val, prim_slot, no_expr())),
	       filename);
    m_classes = append_Classes(m_classes, single_Classes(Int_class));     

    //
    // Bool also has only the "val" slot.
    //
    Class_ Bool_class =
	class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename);
    m_classes = append_Classes(m_classes, single_Classes(Bool_class));

    //
    // The class Str has a number of slots and operations:
    //       val                                  the length of the string
    //       str_field                            the string itself
    //       length() : Int                       returns length of the string
    //       concat(arg: Str) : Str               performs string concatenation
    //       substr(arg: Int, arg2: Int): Str     substring selection
    //       
    Class_ Str_class =
	class_(Str, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       append_Features(
									       single_Features(attr(val, Int, no_expr())),
									       single_Features(attr(str_field, prim_slot, no_expr()))),
							       single_Features(method(length, nil_Formals(), Int, no_expr()))),
					       single_Features(method(concat, 
								      single_Formals(formal(arg, Str)),
								      Str, 
								      no_expr()))),
			       single_Features(method(substr, 
						      append_Formals(single_Formals(formal(arg, Int)), 
								     single_Formals(formal(arg2, Int))),
						      Str, 
						      no_expr()))),
	       filename);
    m_classes = append_Classes(m_classes, single_Classes(Str_class));
}

////////////////////////////////////////////////////////////////////
//
// semant_error is an overloaded function for reporting errors
// during semantic analysis.  There are three versions:
//
//    ostream& ClassTable::semant_error()                
//
//    ostream& ClassTable::semant_error(Class_ c)
//       print line number and filename for `c'
//
//    ostream& ClassTable::semant_error(Symbol filename, tree_node *t)  
//       print a line number and filename
//
///////////////////////////////////////////////////////////////////

ostream& ClassTable::semant_error(Class_ c)
{                                                             
    return semant_error(c->get_filename(),c);
}    

ostream& ClassTable::semant_error(Symbol filename, tree_node *t)
{
    error_stream << filename << ":" << t->get_line_number() << ": ";
    return semant_error();
}

ostream& ClassTable::semant_error()                  
{                                                 
    semant_errors++;                            
    return error_stream;
} 



/*   This is the entry point to the semantic checker.

     Your checker should do the following two things:

     1) Check that the program is semantically correct
     2) Decorate the abstract syntax tree with type information
        by setting the `type' field in each Expression node.
        (see `tree.h')

     You are free to first do 1), make sure you catch all semantic
     errors. Part 2) can be done in a second stage, when you want
     to build mycoolc.
 */
void program_class::semant()
{
    initialize_constants();

    /* ClassTable constructor may do some semantic analysis */
    ClassTable *classtable = new ClassTable(classes);

    /* some semantic analysis code may go here */

    if (classtable->errors()) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }
}


