class Main {
	a: Bool <- false;
	b: Bool <- true;
	init(a: Int, b: Bool, c: String): Main {
		{
		a <- 42;
		new Main;
		}
	};
};

class A inherits Main {
	p: Penis;
	init(a: Int, b: Bool, c: String): A {
		{
			p <- new Penis;
			new A;	
		}
	};
};

class Penis {


};
