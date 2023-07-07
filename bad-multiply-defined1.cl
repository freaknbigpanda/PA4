class C inherits A {

};

class B inherits C {


};

class B inherits IO {


};

class Main {
	main():C {
	  (new C).init(1,true)
	};
};