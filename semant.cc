#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "semant.h"
#include "utilities.h"

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

const InheritanceNode* InheritanceNode::FirstCommonAncestor(const InheritanceNode* otherNode) const
{
    using namespace std;

    if (otherNode == nullptr)
    {
        return nullptr;
    }

    // First build a map of ancestors for the current node (including the current node itself)
    std::set<std::string> ancestors;
    const InheritanceNode* parent = this;
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
            abort(); // Sanity check, we shouldn't ever hit this and if we do it means we haven't properly validated the inheritance graph
            return nullptr;
        }
    }

    return otherParent;
}

bool InheritanceNode::IsChildOfOrEqual(const InheritanceNode *potentialParent) const
{
    const InheritanceNode* parent = this;
    while(parent != nullptr)
    {
        if (parent == potentialParent) return true;
        parent = parent->m_parent;
    }

    return false;
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
        CheckTypes();
    }
}

void ClassTable::install_basic_classes() {

    // The tree package uses these globals to annotate the classes built below.
   // curr_lineno  = 0;
    m_basicClassFilename = stringtable.add_string("<basic class>");

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
	       m_basicClassFilename);
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
	       m_basicClassFilename);
    m_classes = append_Classes(m_classes, single_Classes(IO_class));

    //
    // The Int class has no methods and only a single attribute, the
    // "val" for the integer.
    //
    Class_ Int_class =
	class_(Int,
	       Object,
	       single_Features(attr(val, prim_slot, no_expr())),
	       m_basicClassFilename);
    m_classes = append_Classes(m_classes, single_Classes(Int_class));

    //
    // Bool also has only the "val" slot.
    //
    Class_ Bool_class =
	class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),m_basicClassFilename);
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
	       m_basicClassFilename);
    m_classes = append_Classes(m_classes, single_Classes(Str_class));
}

bool ClassTable::ValidateInheritance()
{
    using namespace std;

    // Just used to check for multiply defined children
    map<string, Class_> allDefinedChildren;

    for(int i = m_classes->first(); m_classes->more(i); i = m_classes->next(i))
    {
        string parentName = m_classes->nth(i)->get_parent()->get_string();
        string childName = m_classes->nth(i)->get_name()->get_string();

        if (childName == SELF_TYPE->get_string())
        {
            semant_error(m_classes->nth(i));
            error_stream << "Redefinition of basic class SELF_TYPE" << endl;
            continue;
        }

        if (parentName == Int->get_string() || parentName == Bool->get_string() || parentName == Str->get_string())
        {
            // class inherits from a basic type
            semant_error(m_classes->nth(i));
            error_stream << "Class " << childName << " inherits from either Int, Bool, or String. This is illegal." << endl;
            continue;
        }

        auto insertResult = allDefinedChildren.find(childName);
        if (insertResult != allDefinedChildren.end())
        {
            Class_ errorClass = m_classes->nth(i);
            // if we have incorrectly redefined a basic class report the user implemented version as incorrect
            if (strcmp(errorClass->get_filename()->get_string(), m_basicClassFilename->get_string()) == 0)
            {
                errorClass = insertResult->second;
            }

            // class is defined multiple times
            semant_error(errorClass);
            error_stream << "Class " << childName << " multiply defined" << endl;
            continue;
        }
        allDefinedChildren.insert({childName, m_classes->nth(i)});

        if (parentName == childName)
        {
            // class inheritis from itself
            semant_error(m_classes->nth(i));
            error_stream << "Class " << childName << " inherits from itself" << endl;
            continue;
        }

        // first check to see if the child already exists
        if (m_inheritanceNodeMap.find(childName) != m_inheritanceNodeMap.end())
        {
            // Don't need to do anything here if the child already exists
        }
        else
        {
            // create a new child node and add it to the map
            m_inheritanceNodeMap[childName] = make_unique<InheritanceNode>(childName);
        }

        // then find out if we have created the parent node already
        if (m_inheritanceNodeMap.find(parentName) != m_inheritanceNodeMap.end())
        {
            // The node already exists, insert child into its child set
            string error_msg;
            bool successfulInsertion = m_inheritanceNodeMap[parentName]->AddChild(m_inheritanceNodeMap[childName].get(), error_msg);
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
            m_inheritanceNodeMap[parentName] = make_unique<InheritanceNode>(parentName);

            // insert child into its child set
            string error_msg;
            bool successfulInsertion = m_inheritanceNodeMap[parentName]->AddChild(m_inheritanceNodeMap[childName].get(), error_msg);
            if (successfulInsertion == false)
            {
                semant_error(m_classes->nth(i));
                error_stream << error_msg << endl;
                break;
            };
        }
    }

    // Every node must have a non-null parent (even object which has a no_class parent)
    for ( auto i = m_inheritanceNodeMap.begin(); i != m_inheritanceNodeMap.end(); ++i)
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
    if (m_inheritanceNodeMap.find("Main") == m_inheritanceNodeMap.end())
    {
        semant_error();
        error_stream << "Class Main is not defined." << endl;
    }

    if (semant_debug)
    {
        // std::string errorString;
        // const InheritanceNode* lub = inheritanceNodeMap["A"]->lub(inheritanceNodeMap["C"].get(), errorString);
        // cout << "The common ancestor of bool and string is " << lub->GetName() << endl;

        cout << "The number of nodes in the inheritance node map is " << m_inheritanceNodeMap[No_class->get_string()]->GetNumDescendants() + 1 << endl;
        cout << "The number of classes that we encountred (parent and child) in the ast is " << m_inheritanceNodeMap.size() << endl;

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

void ClassTable::CheckTypes()
{
    TypeEnvironment typeEnvironment;

    // ***** CLASS GATHER PASS ***** //
    // Gather all declared classes in the symbol table
    for(int i = m_classes->first(); m_classes->more(i); i = m_classes->next(i))
    {
        Class_ currentClass = m_classes->nth(i);

        std::string className = currentClass->get_name()->get_string();
        typeEnvironment.m_symbols.addid(className, currentClass->get_name());

        // populate the class map for use later
        m_classMap[className] = currentClass;
    }

    // ***** METHOD GATHER PASS ***** //
    // Now gather all methods and their formals in the symbol table
    bool mainDefinedInMain = false;
    for(int i = m_classes->first(); m_classes->more(i); i = m_classes->next(i))
    {
        Class_ currentClass = m_classes->nth(i);
        Features features = currentClass->get_features();
        for (int i = features->first(); features->more(i); i = features->next(i))
        {
            Feature feature = features->nth(i);
            if (feature->is_attr()) continue; // we don't care about attributes for this pass

            method_class* methodObject = static_cast<method_class*>(feature);
            MethodKey key = MethodKey(currentClass, methodObject);
            // first check to make sure the method has not been previously defined
            if (typeEnvironment.m_methodMap.find(key) != typeEnvironment.m_methodMap.end()) {
                semant_error(currentClass->get_filename(), methodObject);
                error_stream << "Method defined twice in the same class." << endl;
                continue;
            }

            // then check to make sure that the formals are not redfined in the same method
            Formals formals = methodObject->get_formals();
            std::set<std::string> formalNames;
            for(int i = formals->first(); formals->more(i); i = formals->next(i))
            {
                Formal formal = formals->nth(i);
                if (strcmp(formal->get_name()->get_string(), self->get_string()) == 0)
                {
                    semant_error(currentClass->get_filename(), formal);
                    error_stream << "formal parameter cannot be named self" << endl;
                    continue;
                }

                if (formal->get_type() == SELF_TYPE)
                {
                    semant_error(currentClass->get_filename(), formal);
                    error_stream << "formal parameter type cannot be SELF_TYPE" << endl;
                    continue;
                }

                bool previouslyDefined = formalNames.find(formal->get_name()->get_string()) != formalNames.end();
                if (previouslyDefined)
                {
                    // Formal with same name defined twice - no good
                    semant_error(currentClass->get_filename(), formal);
                    error_stream << "Formal parameter defined twice in the same method" << endl;
                    continue;
                }

                // Add formal name
                formalNames.insert(formal->get_name()->get_string());
            }

            if (strcmp(methodObject->get_name()->get_string(), "main") == 0 &&
                strcmp(currentClass->get_name()->get_string(), "Main") == 0 &&
                formalNames.size() == 0)
            {
                mainDefinedInMain = true;
            }

            typeEnvironment.m_methodMap[key] = MethodInfo(methodObject);
        }
    }

    if (mainDefinedInMain == false)
    {
        semant_error(m_classMap["Main"]);
        error_stream << "main() method that takes no params must be decalred in Main class" << endl;
    }

    // ***** METHOD INHERITANCE CHECK PASS ***** //
    // Now check to make sure that methods defined in child classes conform to the appropiate signature
    for(int i = m_classes->first(); m_classes->more(i); i = m_classes->next(i))
    {
        Class_ currentClass = m_classes->nth(i);

        //todo: This is really unoptimized, we are checking a lot of classes that we don't need to check
        // for example if we have the inheritance relation A -> B -> C -> D and we first check D then
        // we will have checked the entire inheritance tree already but when we encounter C we will still check
        // all of the classes A, B, C. We will end up with a lot of duplicated work for deep hierarchies.
        // I think I can just add some visited metadata to avoid this duplicated work

        // Check to see if the method is improperly redefined in child classes
        std::string className = currentClass->get_name()->get_string();
        Features features = currentClass->get_features();
        for (int i = features->first(); features->more(i); i = features->next(i))
        {
            Feature feature = features->nth(i);
            if (feature->is_attr()) continue; // we don't care about attributes for this pass

            method_class* methodObject = static_cast<method_class*>(feature);
            const InheritanceNode* parent = m_inheritanceNodeMap[className].get()->GetParent();
            MethodKey childKey = MethodKey(className, methodObject->get_name()->get_string());
            while (parent != nullptr)
            {
                MethodKey parentKey = MethodKey(parent->GetName(), methodObject->get_name()->get_string());
                if (typeEnvironment.m_methodMap.find(parentKey) != typeEnvironment.m_methodMap.end())
                {
                    // We have found a redefinition in a parent class, need to check to make sure that the number and types of formals are the same
                    if (typeEnvironment.m_methodMap[parentKey] != typeEnvironment.m_methodMap[childKey])
                    {
                        semant_error(currentClass->get_filename(), methodObject);
                        error_stream << "Method redefined in " << className << " does not match parent class method signature" << endl;
                        break;
                    }
                }
                parent = parent->GetParent();
            }
        }
    }

    // ***** ATTRIBUTE GATHER & FEATURE TYPE CHECK PASS ***** //
    for(int i = m_classes->first(); m_classes->more(i); i = m_classes->next(i))
    {
        Class_ currentClass = m_classes->nth(i);
        std::string className = currentClass->get_name()->get_string();

        typeEnvironment.EnterScope();
        typeEnvironment.m_currentClass = currentClass;

        // For each class hierarchy loop through all the parent classes and add their attributes to the symbol table
        //  while checking to see if they are defined twice
        const InheritanceNode* parentClassNode = m_inheritanceNodeMap[className].get();
        while (parentClassNode != nullptr && parentClassNode->GetName() !=  No_class->get_string()) {
            Class_ parentClass = m_classMap[parentClassNode->GetName()];
            if (parentClass == nullptr) {
                abort(); // Just for debug, this should never happen
            }

            // Atrribute gather and dedupe
            Features features = parentClass->get_features();
            for (int i = features->first(); features->more(i); i = features->next(i))
            {
                Feature feature = features->nth(i);
                // Only gathering attributes here
                if (feature->is_attr() == false) continue;

                std::string featureName = feature->get_name()->get_string();

                if (featureName == "self")
                {
                    // Attribute with same name defined twice - continue to next attribute
                    semant_error(parentClass->get_filename(), feature);
                    error_stream << "'self' cannot be the name of an attribute." << endl;
                    continue;
                }

                // First make sure that the attribute is not previously defined, note that we have already done this for methods previously
                if (typeEnvironment.m_symbols.probe(featureName) != nullptr)
                {
                    // Attribute with same name defined twice - continue to next attribute
                    semant_error(parentClass->get_filename(), feature);
                    error_stream << "Attribute redefined in the same class or class hierarchy." << endl;
                    continue;
                }

                // Attribute not previously defined so we can add it to the symbol table
                typeEnvironment.m_symbols.addid(featureName, feature->get_type());
            }

            parentClassNode = parentClassNode->GetParent();
        }

        // Now we have a complete list of attributes in our symbol table, continue to type checking

        // Feature type checking
        Features features = currentClass->get_features();
        for (int i = features->first(); features->more(i); i = features->next(i))
        {
            Feature feature = features->nth(i);
            std::string featureName = feature->get_name()->get_string();

            typeEnvironment.EnterScope(); // enter scope in case we are processing a method

            if (feature->is_attr() == false)
            {
                // For methods we want to add all the formals to the symbol table
                Formals formals = dynamic_cast<method_class*>(feature)->get_formals();
                for(int i = formals->first(); formals->more(i); i = formals->next(i))
                {
                    Formal formal = formals->nth(i);
                    typeEnvironment.m_symbols.addid(formal->get_name()->get_string(), formal->get_type());
                }
            }

            //Recursively type check the expression if it is not NoExpr class
            Expression expression = feature->get_expression();
            if (expression->get_expr_type() != ExpressionType::NoExpr)
            {
                Symbol expressionType = TypeCheckExpression(typeEnvironment, expression);

                // todo: really gross SELF_TYPE special cases... I should clean this up
                Symbol featureType = feature->get_type();
                if (feature->is_attr() == false && featureType == SELF_TYPE && expressionType != SELF_TYPE)
                {
                    semant_error(typeEnvironment.m_currentClass->get_filename(), feature);
                    error_stream << "Methods with return type SELF_TYPE must return self" << endl;
                }
                else if (expressionType == nullptr || IsClassChildOfClassOrEqual(expressionType, featureType, typeEnvironment) == false)
                {
                    semant_error(typeEnvironment.m_currentClass->get_filename(), feature);
                    std::string errorString = feature->is_attr() ? "Attribute initialization type mismatch" : "Method expression and return type mismatch";
                    error_stream << errorString << endl;
                }
            }

            typeEnvironment.ExitScope(); // exit method scope
        }

        typeEnvironment.m_currentClass = nullptr;
        typeEnvironment.ExitScope();
    }
}

bool ClassTable::IsClassChildOfClassOrEqual(Symbol childClass, Symbol potentialParentClass, const TypeEnvironment& typeEnvironment)
{
    if (childClass == SELF_TYPE && potentialParentClass == SELF_TYPE)
    {
        // SELF_TYPE will always be from the same class so this is always true
        return true;
    }

    if (potentialParentClass == SELF_TYPE) {
        // T <= SELF_TYPEc should always be false, see self type lecture
        return false;
    }

    if (childClass == SELF_TYPE) {
        // SELF_TYPEc <= T if C <= T
        childClass = typeEnvironment.m_currentClass->get_name();
    }

    if (childClass == nullptr || potentialParentClass == nullptr) {
        return false;
    }

    InheritanceNode* childNode = m_inheritanceNodeMap[childClass->get_string()].get();
    InheritanceNode* parentNode = m_inheritanceNodeMap[potentialParentClass->get_string()].get();

    if (childNode == nullptr || parentNode == nullptr) {
        return false;
    }

    return childNode->IsChildOfOrEqual(parentNode);
}

Symbol ClassTable::FirstCommonAncestor(Symbol first, Symbol second, const TypeEnvironment& typeEnvironment)
{
    if (first == SELF_TYPE && second == SELF_TYPE)
    {
        return SELF_TYPE;
    }

    if (first == SELF_TYPE)
    {
        first = typeEnvironment.m_currentClass->get_name();
    }
    else if (second == SELF_TYPE)
    {
        second = typeEnvironment.m_currentClass->get_name();
    }

    const InheritanceNode* thenTypeNode = m_inheritanceNodeMap[first->get_string()].get();
    const InheritanceNode* elseTypeNode = m_inheritanceNodeMap[second->get_string()].get();

    const InheritanceNode* commonAncestor = thenTypeNode->FirstCommonAncestor(elseTypeNode);

    return m_classMap[commonAncestor->GetName()]->get_name();
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

            // The assign expression is accepted as long as it assigning a subclass of the declared identifier type
            Symbol parentType = typeEnvironment.m_symbols.lookup(name->get_string());
            if (IsClassChildOfClassOrEqual(exprType, parentType, typeEnvironment) == false)
            {
                semant_error(typeEnvironment.m_currentClass->get_filename(), expression);
                error_stream << "Assignment expression has a static type that does not match the identifier, or the identifier type is unknown" << endl;
                break;
            }
            expressionType = exprType;
            break;
        }
        case ExpressionType::Block:
        {
            Expressions expressions = static_cast<block_class*>(expression)->get_body();
            for(int i = expressions->first(); expressions->more(i); i = expressions->next(i))
            {
                Expression blockExpr = expressions->nth(i);
                expressionType = TypeCheckExpression(typeEnvironment, blockExpr);
            }
            break;
        }
        case ExpressionType::Conditional:
        {
            cond_class* conditional = static_cast<cond_class*>(expression);
            Symbol predType = TypeCheckExpression(typeEnvironment, conditional->get_pred());

            if (predType->get_string() != Bool->get_string())
            {
                semant_error(typeEnvironment.m_currentClass->get_filename(), expression);
                error_stream << "Conditional statement predicate must be of static type Boolean" << endl;
                break;
            }

            Symbol thenType = TypeCheckExpression(typeEnvironment, conditional->get_then());
            Symbol elseType = TypeCheckExpression(typeEnvironment, conditional->get_else());
            expressionType = FirstCommonAncestor(thenType, elseType, typeEnvironment);
            break;
        }
        case ExpressionType::Dispatch:
        case ExpressionType::StaticDispatch:
        {
            // <expr>.<id>(<expr>,...,<expr>)
            // <id>(<expr>,...,<expr>) aka self.<id>(<expr>,...,<expr>)
            // <expr>@<type>.id(<expr>,...,<expr>)

            Symbol identifierExprType = TypeCheckExpression(typeEnvironment, expression->get_dispatch_id_expr());
            bool isIdentifierTypeSelfType = identifierExprType == SELF_TYPE;
            if (isIdentifierTypeSelfType) identifierExprType = typeEnvironment.m_currentClass->get_name();
            Symbol subclassName = expression->get_dispatch_subclass_type();

            bool isStaticDispatch = subclassName != nullptr;
            if (isStaticDispatch && subclassName == SELF_TYPE)
            {
                semant_error(typeEnvironment.m_currentClass->get_filename(), expression);
                error_stream << "SELF_TYPE cannot be used in static dispatch expression" << endl;
                return nullptr;
            }

            // First check to make sure that the static type of the expr conforms to the subclass type we are dispatching too
            if (isStaticDispatch && IsClassChildOfClassOrEqual(identifierExprType, subclassName, typeEnvironment) == false)
            {
                semant_error(typeEnvironment.m_currentClass->get_filename(), expression);
                error_stream << "The dispatch expression static type of " << identifierExprType->get_string() << " is not a subclass of " << subclassName->get_string() << endl;
                return nullptr;
            }

            Symbol baseClassType = isStaticDispatch ? subclassName : identifierExprType;

            // Then check to make sure that a method with that name exists on the class or its parents
            bool methodFound = false;
            MethodInfo foundMethodInfo;
            std::string methodName = expression->get_dispatch_method_name()->get_string();
            while (baseClassType != nullptr)
            {
                MethodKey methodKey = MethodKey(baseClassType->get_string(), methodName);
                if (typeEnvironment.m_methodMap.find(methodKey) != typeEnvironment.m_methodMap.end())
                {
                    foundMethodInfo = typeEnvironment.m_methodMap[methodKey];
                    methodFound = true;
                    break;
                }
                const InheritanceNode* parentNode = m_inheritanceNodeMap[baseClassType->get_string()]->GetParent();
                std::string parentClassName = parentNode->GetName();

                if (parentClassName == No_class->get_string()) break; // reached the top of the inheritance hierarchy

                baseClassType = m_classMap[parentClassName]->get_name();
            }

            if (methodFound == false)
            {
                semant_error(typeEnvironment.m_currentClass->get_filename(), expression);
                error_stream << "Tried to call method that was not defined in the specified class hierarchy" << endl;
                return nullptr;
            }

            // Then gather all of the method formal types
            Expressions formalExpressions = expression->get_dispatch_param_expressions();
            for(int i = formalExpressions->first(); formalExpressions->more(i); i = formalExpressions->next(i))
            {
                Expression formalExpression = formalExpressions->nth(i);
                Symbol formalExpressionType = TypeCheckExpression(typeEnvironment, formalExpression);
                if (formalExpressionType == nullptr)
                {
                    semant_error(typeEnvironment.m_currentClass->get_filename(), formalExpression);
                    error_stream << "Formal has unknown type in dispatch expression" << endl;
                    return nullptr;
                }
                
                Symbol foundFormalExpressionType = foundMethodInfo.GetFormalTypes()[i];
                if (IsClassChildOfClassOrEqual(formalExpressionType, foundFormalExpressionType, typeEnvironment) == false)
                {
                    semant_error(typeEnvironment.m_currentClass->get_filename(), expression);
                    error_stream << "Method signature in dispatch expression does not match declaration" << endl;
                }
            }

            if (foundMethodInfo.GetReturnType() == SELF_TYPE)
            {
                expressionType = isIdentifierTypeSelfType ? SELF_TYPE : identifierExprType;
            }
            else
            {
                expressionType = foundMethodInfo.GetReturnType();
            }
            break;
        }
        case ExpressionType::New:
        {
            expressionType = static_cast<new__class*>(expression)->get_type_name();
            break;
        }
        case ExpressionType::Let:
        {
            let_class* letExpr = static_cast<let_class*>(expression);

            Symbol letId = letExpr->get_let_id();
            if (strcmp(letId->get_string(), self->get_string()) == 0)
            {
                semant_error(typeEnvironment.m_currentClass->get_filename(), expression);
                error_stream << "let method identier cannot be named self" << endl;
            }

            Symbol letTypeDecl = letExpr->get_let_type_decl();
            Expression letInit = letExpr->get_let_init();
            Expression letBody = letExpr->get_let_body();	

            if (letInit->get_expr_type() != ExpressionType::NoExpr)
            {
                Symbol initType = TypeCheckExpression(typeEnvironment, letInit);
                if (IsClassChildOfClassOrEqual(initType, letTypeDecl, typeEnvironment) == false)
                {
                    semant_error(typeEnvironment.m_currentClass->get_filename(), expression);
                    error_stream << "let-init method static type does not match type declaration" << endl;
                }
            }
            
            typeEnvironment.EnterScope(); // let scope
            
            typeEnvironment.m_symbols.addid(letId->get_string(), letTypeDecl);
            expressionType = TypeCheckExpression(typeEnvironment, letBody);

            typeEnvironment.ExitScope();
            break;
        }
        case ExpressionType::TypeCase:
        {
            typcase_class* typecaseExpr = static_cast<typcase_class*>(expression);
            Cases cases = typecaseExpr->get_cases();
            Expression caseExpr = typecaseExpr->get_case_expr();

            Symbol caseExprType = TypeCheckExpression(typeEnvironment, caseExpr); 
            std::set<Symbol> branchTypes;
            for(int i = cases->first(); cases->more(i); i = cases->next(i))
            {
                Case caseObj = cases->nth(i);
                branch_class* caseBranch = static_cast<branch_class*>(caseObj);

                Symbol idName = caseBranch->get_name();
                if (strcmp(idName->get_string(), self->get_string()) == 0)
                {
                    semant_error(typeEnvironment.m_currentClass->get_filename(), expression);
                    error_stream << "case branch identier cannot be named self" << endl;
                }
                
                Symbol typeDecl = caseBranch->get_type();
                Expression branchExpr = caseBranch->get_expr();

                if (branchTypes.find(typeDecl) != branchTypes.end())
                {
                    semant_error(typeEnvironment.m_currentClass->get_filename(), branchExpr);
                    error_stream << "Branches in a case statement with the same type are illegal" << endl;
                    return nullptr;
                }
                branchTypes.insert(typeDecl);
                
                typeEnvironment.EnterScope(); // case scope

                typeEnvironment.m_symbols.addid(idName->get_string(), typeDecl);
                Symbol currentBranchExprType = TypeCheckExpression(typeEnvironment, branchExpr);

                if (expressionType != nullptr) 
                {
                    expressionType = FirstCommonAncestor(expressionType, currentBranchExprType, typeEnvironment);
                } 
                else
                {
                    expressionType = currentBranchExprType;
                }

                typeEnvironment.ExitScope();
            }

            break;
        }
        case ExpressionType::Loop:
        {
            loop_class* loopExpr = static_cast<loop_class*>(expression);
            Expression loopPred = loopExpr->get_pred();
            Expression loopBody = loopExpr->get_body();

            if (TypeCheckExpression(typeEnvironment, loopPred) != Bool)
            {
                semant_error(typeEnvironment.m_currentClass->get_filename(), loopExpr);
                error_stream << "Loop predicate must be of type Bool" << endl;
            }
            else if (TypeCheckExpression(typeEnvironment, loopBody) != nullptr)
            {
                expressionType = Object;
            }
            break;
        }
        case ExpressionType::IsVoid:
        {
            if (TypeCheckExpression(typeEnvironment, expression->get_rhs()) != nullptr)
            {
                expressionType = Bool;
            }

            break;
        }
        case ExpressionType::Comp:
        {
            if (TypeCheckExpression(typeEnvironment, expression->get_rhs()) != Bool)
            {
                semant_error(typeEnvironment.m_currentClass->get_filename(), expression->get_rhs());
                error_stream << "not operator only takes expressions of type Bool" << endl;
            }
            else
            {
                expressionType = Bool;
            }

            break;
        }
        case ExpressionType::Neg:
        {
            if (TypeCheckExpression(typeEnvironment, expression->get_rhs()) != Int)
            {
                semant_error(typeEnvironment.m_currentClass->get_filename(), expression->get_rhs());
                error_stream << "neg operator only takes expressions of type Int" << endl;
            }
            else
            {
                expressionType = Int;
            }

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

            if (symbolName == self->get_string())
            {
                expressionType = SELF_TYPE;
            }
            else
            {
                expressionType = typeEnvironment.m_symbols.lookup(symbolName);
                if (expressionType == nullptr)
                {
                    semant_error(typeEnvironment.m_currentClass->get_filename(), expression);
                    error_stream << "Identifier not defined in this scope" << endl;
                }
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
                    semant_error(typeEnvironment.m_currentClass->get_filename(), expression);
                    error_stream << "Comparison can only be made between two basic types" << endl;
                    return nullptr;
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
                semant_error(typeEnvironment.m_currentClass->get_filename(), expression);
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
            abort(); // All expression types must be handled
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


