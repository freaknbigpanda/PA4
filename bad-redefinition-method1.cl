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
	init(a: Int, b: Int, c: String): Main {
		{
			p <- new Penis;
			new A;	
		}
	};
};

class Penis {


};
