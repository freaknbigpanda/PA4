class C inherits A {

};

class B inherits C {


};

class A inherits B {


};

class Main {
	main():C {
	  (new C).init(1,true)
	};
};
