class C inherits A {
	a : Int;
	b : Bool;
	init(x : Int, y : Bool) : C {
           {
		a <- x;
		b <- y;
		self;
           }
	};
};

class B inherits C {


};

class A {


};

class Main {
	main():C {
	  (new C).init(1,true)
	};
};
