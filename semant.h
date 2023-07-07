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

    bool AddChild(InheritanceNode*);
    int GetNumChildren() { return m_children.size(); }
    int GetNumDescendants() { return m_numDescendants; } 
    bool GetCycleDetected() { return m_cycleDetected; }

private:
    std::string m_name;
    std::set<InheritanceNode*> m_children;
    InheritanceNode* m_parent;
    int m_numDescendants = 0;
    bool m_cycleDetected = false;
    bool m_visited = false;
};

// This is a structure that may be used to contain the semantic
// information such as the inheritance graph.  You may use it or not as
// you like: it is only here to provide a container for the supplied
// methods.

class ClassTable {
private:
  int semant_errors;
  void install_basic_classes();
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

