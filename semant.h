#ifndef SEMANT_H_
#define SEMANT_H_

#include <assert.h>
#include <iostream>  
#include "cool-tree.h"
#include "stringtab.h"
#include "symtab.h"
#include "list.h"

#include <set>
#include <map>
#include <memory>

#define TRUE 1
#define FALSE 0

class ClassTable;
typedef ClassTable *ClassTableP;

class InheritanceNode
{
public:
    InheritanceNode(const std::string& name) : m_name(name) {};

    const InheritanceNode* FirstCommonAncestor(const InheritanceNode* otherNode, std::string& errorStr) const;
    bool IsChildOfOrEqual(const InheritanceNode* potentialParent) const;
    bool AddChild(InheritanceNode*, std::string&);
    bool HasParent() const { return m_parent != nullptr; } 
    int GetNumChildren() const { return m_children.size(); }
    int GetNumDescendants() const { return m_numDescendants; } 
    std::string GetName() const { return m_name; }

private:
    std::string m_name;
    std::set<InheritanceNode*> m_children;
    InheritanceNode* m_parent; 
    int m_numDescendants = 0;
    bool m_visited = false;
};

struct TypeEnvironment
{
  TypeEnvironment() { EnterScope(); }

  void EnterScope() { symbols.enterscope(); methods.enterscope(); }
  void ExitScope() { symbols.exitscope(); methods.exitscope(); }

  SymbolTable<std::string, Entry> symbols;
  SymbolTable<std::string, Entry> methods;
  Class_ currentClass;
};

// This is a structure that may be used to contain the semantic
// information such as the inheritance graph.  You may use it or not as
// you like: it is only here to provide a container for the supplied
// methods.

typedef std::map<std::string, std::unique_ptr<InheritanceNode>> InheritanceNodeMap;

class ClassTable {
private:
  int semant_errors;
  void install_basic_classes();
  bool ValidateInheritance();
  void CheckTypes();
  bool IsClassChildOfClassOrEqual(Symbol childClass, Symbol potentialParentClass);
  Symbol TypeCheckExpression(TypeEnvironment& typeEnvironment, Expression expression);

  ostream& error_stream;
  Classes m_classes;
  InheritanceNodeMap m_inheritanceNodeMap;

public:
  ClassTable(Classes);
  int errors() { return semant_errors; }
  ostream& semant_error();
  ostream& semant_error(Class_ c);
  ostream& semant_error(Symbol filename, tree_node *t);
};


#endif

