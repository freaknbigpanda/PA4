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

const InheritanceNode* InheritanceNode::lub(const InheritanceNode* otherNode, std::string& errorString)
{
    using namespace std;

    if (otherNode == nullptr)
    {
        std::stringstream stream;
        stream << "Passed nullptr to lub function" << endl;
        errorString = stream.str();
        return nullptr;
    }
    
    // First build a map of ancestors for the current node (including the current node itself)
    std::set<std::string> ancestors;
    InheritanceNode* parent = this;
    while(parent != nullptr)
    {
        ancestors.insert(parent->GetName());
        parent = parent->m_parent;
    }

    // Then iterate up the inheritance graph from the othernode and return the first match
    const InheritanceNode* otherParent = otherNode;
    while(ancestors.find(otherParent->m_name) == ancestors.end())
    {
        otherParent = otherParent->m_parent;

        if (otherParent == nullptr)
        {
            std::stringstream stream;
            stream << "Class " << otherNode->m_name << " and " << m_name << " have no common ancestor" << endl;
            errorString = stream.str();
            return nullptr;
        }
    }

    return otherParent;
}

bool InheritanceNode::AddChild(InheritanceNode *newChild, std::string &errorString)
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

    if (ValidateInheritance())
    {
        if (CheckTypes())
        {
            std::cout << "Success!" << std::endl;
        }
        else
        {
            error_stream << "Types are messed up" << endl;
        }

    }
    else
    {
        error_stream << "Inheritance graph is messed up" << endl;
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

bool ClassTable::ValidateInheritance()
{
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
        // std::string errorString;
        // const InheritanceNode* lub = inheritanceNodeMap["A"]->lub(inheritanceNodeMap["C"].get(), errorString);
        // cout << "The common ancestor of bool and string is " << lub->GetName() << endl;

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

    return semant_errors == 0;
}

// I need some method with a signature like ClassTable::VerifyExpression('environment', expression)

bool ClassTable::CheckTypes()
{
    TypeEnvironment typeEnvironment;
    bool success = true;

    // Gather all declared classes in the symbol table
    for(int i = m_classes->first(); m_classes->more(i); i = m_classes->next(i))
    { 
        std::string className = m_classes->nth(i)->get_name()->get_string();
        typeEnvironment.symbols.addid(className, m_classes->nth(i)->get_name());
    }

    // Really need to add all methods to the symbol table here.
    // I don't need to type check the expressions but I need to add all class methods to the symbol table
    // Because other expressions and attributes may refer to them
    // 

    // For each class gather attributes into the symbol table and mark their types in the ast
    for(int i = m_classes->first(); m_classes->more(i); i = m_classes->next(i))
    { 
        typeEnvironment.EnterScope();
        typeEnvironment.currentClass = m_classes->nth(i);

        // First add feature to the symbol table and type check the expressions
        Features features = m_classes->nth(i)->get_features();
        for (int i = features->first(); features->more(i); i = features->next(i))
        {
            // Need to exit method scope if the last feature was a method
            if (i-1 >= 0 && features->nth(i-1)->is_attr() == false) 
            {
                typeEnvironment.ExitScope();
            }

            Feature feature = features->nth(i);
            std::string featureName = feature->get_name()->get_string();
            // First make sure that the feature is not previously defined
            bool previouslyDefined = feature->is_attr() ? typeEnvironment.symbols.probe(featureName) != nullptr :
            typeEnvironment.methods.probe(featureName) != nullptr;

            if (previouslyDefined == false)
            {
                // Attribute not previously defined so we can add it to the symbol table
                if (feature->is_attr())
                {
                    typeEnvironment.symbols.addid(featureName, feature->get_type());
                }
                else
                {
                    // Enter method scope
                    typeEnvironment.EnterScope();

                    typeEnvironment.methods.addid(featureName, feature->get_type());

                    Formals formals = static_cast<method_class*>(feature)->get_formals();

                    for(int i = formals->first(); formals->more(i); i = formals->next(i))
                    {
                        Formal formal = formals->nth(i);
                        bool previouslyDefined = typeEnvironment.symbols.probe(formal->get_name()->get_string());
                        if (previouslyDefined)
                        {
                            // Formal with same name defined twice - no good
                            semant_error(typeEnvironment.currentClass->get_filename(), feature);
                            error_stream << "Formal parameter defined twice in the same method" << endl;
                            success = false;
                            continue; // try to resume semantic analysic at the next feature
                        }

                        // Add formal
                        typeEnvironment.symbols.addid(formal->get_name()->get_string(), formal->get_type());
                    }

                } 

                // Recursively type check the expression if it is not NoExpr class
                Expression expression = feature->get_expression();
                if (expression->get_expr_type() != ExpressionType::NoExpr) {
                    Symbol expressionType = TypeCheckExpression(typeEnvironment, expression);

                    if (expressionType == nullptr)
                    {
                        success = false;
                        break;
                    }

                    if (feature->get_type() != expressionType)
                    {
                        semant_error(typeEnvironment.currentClass->get_filename(), feature);
                        std::string errorString = feature->is_attr() ? "Attribute initialization type mismatch" : "Method return type doesn't match method expression type";
                        error_stream << errorString << endl;
                        success = false;
                        break;
                    }
                }
            }
            else
            {
                // Attribute with same name defined twice - no good
                semant_error(typeEnvironment.currentClass->get_filename(), feature);
                std::string featureType = feature->is_attr() ? "Attribute" : "Method";
                error_stream << featureType << " defined twice in the same class." << endl;
                success = false;
                break; // don't continue processing features if there is an error
                // todo: verify that this behavior is correct
            }
        }

        typeEnvironment.ExitScope();
    }

    return success;
}

Symbol ClassTable::TypeCheckExpression(TypeEnvironment& typeEnvironment,  Expression expression)
{
    Symbol expressionType = nullptr;
    switch(expression->get_expr_type())
    {
        case ExpressionType::Assign:
        {
            Symbol name = static_cast<assign_class*>(expression)->get_symbol_name();
            Expression assignExpr = static_cast<assign_class*>(expression)->get_expr();
            Symbol exprType = TypeCheckExpression(typeEnvironment, assignExpr);
            if (exprType != typeEnvironment.symbols.lookup(name->get_string())) 
            {
                semant_error(typeEnvironment.currentClass->get_filename(), expression);
                error_stream << "Assignment expression has a static type that does not match the identifier" << endl;
            }
            expressionType = exprType;
            break;
        }
        case ExpressionType::Block:
        {
            // todo: I guess block expressions don't modify the scope??
            //typeEnvironment.EnterScope();

            Expressions expressions = static_cast<block_class*>(expression)->get_body();
            for(int i = expressions->first(); expressions->more(i); i = expressions->next(i))
            {
                Expression blockExpr = expressions->nth(i);
                expressionType = TypeCheckExpression(typeEnvironment, blockExpr);
            }

            //typeEnvironment.ExitScope();
            break;
        }
        case ExpressionType::IntConst:
        {
            expressionType = Int;
            break;
        }
        case ExpressionType::BoolConst:
        {
            expressionType = Bool;
            break;
        }
        case ExpressionType::StringConst:
        {
            expressionType = Str;
            break;
        }
        case ExpressionType::Object:
        {
            std::string symbolName = static_cast<object_class*>(expression)->get_name()->get_string();
            expressionType = typeEnvironment.symbols.lookup(symbolName);
            if (expressionType == nullptr)
            {
                semant_error(typeEnvironment.currentClass->get_filename(), expression);
                error_stream << "Identifier not defined in this scope" << endl;
            }
            break;
        }
        case ExpressionType::Eq:
        {
            Symbol lhs = TypeCheckExpression(typeEnvironment, expression->get_lhs());
            Symbol rhs = TypeCheckExpression(typeEnvironment, expression->get_rhs());
            if (lhs != nullptr && rhs != nullptr)
            {
                if ((lhs == Int && rhs != Int) || (lhs == Str && rhs != Str)|| (lhs == Bool && rhs != Bool))
                {
                    semant_error(typeEnvironment.currentClass->get_filename(), expression);
                    error_stream << "Comparison can only be made between two basic types" << endl;
                    break;
                }
                expressionType = Bool;
            }
            break;
        }
        case ExpressionType::Lt:
        case ExpressionType::Leq:
        case ExpressionType::Plus:
        case ExpressionType::Sub:
        case ExpressionType::Divide:
        case ExpressionType::Mul:
        {
            Symbol lhs = TypeCheckExpression(typeEnvironment, expression->get_lhs());
            Symbol rhs = TypeCheckExpression(typeEnvironment, expression->get_rhs());
            if (lhs == nullptr || lhs != Int || rhs == nullptr || rhs != Int)
            {
                semant_error(typeEnvironment.currentClass->get_filename(), expression);
                error_stream << "Opeation is only valid between two Ints" << endl;
                expressionType = nullptr;
                break;
            }

            if (expression->get_expr_type() == ExpressionType::Lt || expression->get_expr_type() == ExpressionType::Leq)
            {
                expressionType = Bool;
            }
            else
            {
                expressionType = Int;
            }
            break;
        }
        default:
        {
            //todo: error case

        }
    }

    if (expressionType != nullptr)
    {
        expression->set_type(expressionType);
    }
    return expressionType;
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


