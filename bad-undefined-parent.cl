class A inherits B {

};

class B inherits C {


};

class C inherits D {


};

class Main {
	main():C {
	  (new C).init(1,true)
	};
};

