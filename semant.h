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
#include <vector>

#define TRUE 1
#define FALSE 0

class ClassTable;
typedef ClassTable *ClassTableP;

class InheritanceNode
{
public:
    InheritanceNode(const std::string& name) : m_name(name) {};

    const InheritanceNode* FirstCommonAncestor(const InheritanceNode* otherNode) const;
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

class MethodInfo {
public:
  MethodInfo() = default;
  MethodInfo(const MethodInfo& other) = default;
  MethodInfo(Symbol returnType, const std::vector<Symbol>& formalTypes): m_returnType(returnType), m_formalTypes(formalTypes) {}
  MethodInfo(method_class* methodObject)
  {
    m_returnType = methodObject->get_type();

    Formals formals = methodObject->get_formals();
    for(int i = formals->first(); formals->more(i); i = formals->next(i))
    { 
      m_formalTypes.push_back(formals->nth(i)->get_type());
    }
  }

  Symbol GetReturnType() const { return m_returnType; }
  const std::vector<Symbol> GetFormalTypes() const { return m_formalTypes; }

  bool operator ==(const MethodInfo& other) const {
    return m_returnType == other.m_returnType && m_formalTypes == other.m_formalTypes;
  }

  bool operator !=(const MethodInfo& other) const {
    return !(*this == other);
  }

private:
  Symbol m_returnType = nullptr;
  std::vector<Symbol> m_formalTypes;
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
  bool IsClassChildOfClassOrEqual(Symbol childClass, Symbol potentialParentClass, const TypeEnvironment& typeEnvironment);
  Symbol FirstCommonAncestor(Symbol C, Symbol T, const TypeEnvironment &TypeEnvironment);
  Symbol TypeCheckExpression(TypeEnvironment& typeEnvironment, Expression expression);

  ostream& error_stream;
  Classes m_classes;
  InheritanceNodeMap m_inheritanceNodeMap;

  // todo: pretty sure this can be removed if I include Symbol points to class type objects in the InheritanceNodes
  std::map<std::string, Class_> m_classMap; // Used in later passes for quick lookup by class name

  Symbol m_basicClassFilename;
public:
  ClassTable(Classes);
  int errors() { return semant_errors; }
  ostream& semant_error();
  ostream& semant_error(Class_ c);
  ostream& semant_error(Symbol filename, tree_node *t);
};


#endif

