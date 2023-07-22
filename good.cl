class Main {
	a: Bool <- false;
	b: Bool <- true;
	init(): Main {
		{
		a <- true;
		new Main;
		}
	};
};

class A inherits Main {
	p: Main;
	init(): A {
		{
			p <- new Penis;
			new A;	
		}
	};
};

class Penis inherits A {


};

