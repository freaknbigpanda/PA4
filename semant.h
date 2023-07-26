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
#include <utility>

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
    const InheritanceNode* GetParent() const { return m_parent; }
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

struct MethodInfo {
  MethodInfo() = default;
  MethodInfo(const MethodInfo& other) = default;
  MethodInfo(method_class* methodObject): 
    m_formals(methodObject->get_formals()), m_returnType(methodObject->get_type()) {}

  
  Formals m_formals = nullptr;
  Symbol m_returnType = nullptr;

  bool operator ==(const MethodInfo& other) const {
    for(int i = m_formals->first(); m_formals->more(i); i = m_formals->next(i))
    { 
        if (other.m_formals->nth(i) == nullptr || m_formals->nth(i)->get_type() != other.m_formals->nth(i)->get_type())
        {
          return false;
        }
    }
    return m_returnType == other.m_returnType;
  }

  bool operator !=(const MethodInfo& other) const {
    return !(*this == other);
  }
};

// class to store a unqiue method key
class MethodKey {
public:
  MethodKey() = default;
  MethodKey(const MethodKey& other) = default;
  MethodKey(Class_ classObject, method_class* methodObject): 
    m_key({ classObject->get_name()->get_string(), methodObject->get_name()->get_string() }) {}
  MethodKey(std::string className, std::string methodName): m_key({ className, methodName }) {}

  bool operator <(const MethodKey& other) const {
    return m_key < other.m_key;
  }

private:
  std::pair<std::string, std::string> m_key;
};

// Map from class name + method name to the list of formals for that method
typedef std::map<MethodKey, MethodInfo> MethodMap;

struct TypeEnvironment
{
  TypeEnvironment() { EnterScope(); }

  // todo: I should use a destructor here so that I don't need to explicitly call exitscope, scope would be exited when the item is destructed
  void EnterScope() { m_symbols.enterscope(); }
  void ExitScope() { m_symbols.exitscope(); }

  SymbolTable<std::string, Entry> m_symbols;
  MethodMap m_methodMap;
  Class_ m_currentClass = nullptr;
};

// This is a structure that may be used to contain the semantic
// information such as the inheritance graph.  You may use it or not as
// you like: it is only here to provide a container for the supplied
// methods.

// Map from class name to the entry in the inheritance node graph for that class
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

