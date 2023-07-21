#ifndef SEMANT_H_
#define SEMANT_H_

#include <assert.h>
#include <iostream>  
#include "cool-tree.h"
#include "stringtab.h"
#include "symtab.h"
#include "list.h"

#include <set>

#define TRUE 1
#define FALSE 0

class ClassTable;
typedef ClassTable *ClassTableP;

class InheritanceNode
{
public:
    InheritanceNode(const std::string& name) : m_name(name) {};

    const InheritanceNode* lub(const InheritanceNode* otherNode, std::string& errorStr);
    bool AddChild(InheritanceNode*, std::string&);
    bool HasParent() { return m_parent != nullptr; } 
    bool HasParent() const { return m_parent != nullptr; } 
    int GetNumChildren() { return m_children.size(); }
    int GetNumChildren() const { return m_children.size(); }
    int GetNumDescendants() { return m_numDescendants; } 
    int GetNumDescendants() const { return m_numDescendants; } 
    std::string GetName() { return m_name; }
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

class ClassTable {
private:
  int semant_errors;
  void install_basic_classes();
  bool ValidateInheritance();
  bool CheckTypes();
  Symbol TypeCheckExpression(TypeEnvironment& typeEnvironment, Expression expression);

  ostream& error_stream;
  Classes m_classes;

public:
  ClassTable(Classes);
  int errors() { return semant_errors; }
  ostream& semant_error();
  ostream& semant_error(Class_ c);
  ostream& semant_error(Symbol filename, tree_node *t);
};


#endif

