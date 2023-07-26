class Main {
	aaa: Bool <- false;
	b: Bool <- aaa;
	c: Bool <- c;
	init(a: Int, b: Bool, c: String): Main {
		{
		a <- 42;
		new SELF_TYPE;
		}
	};
	main(): Int {
		42
	};
	testAgain(a: Int, b: Bool, c: String): SELF_TYPE {
		new SELF_TYPE
	};
};

class A inherits Main {
	p: Penis;
	init(a: Int, b: Bool, c: String): Main {
		{
			p <- new Penis;
			new A;	
		}
	};
	testAgain(a: Int, b: Bool, c: String): SELF_TYPE {
		new SELF_TYPE
	};
};

class Penis inherits A {
	pp: SELF_TYPE;
	bbb: Int;
	test(): Bool {
		{
			aaa <- true;
			p <- new Penis;
			pp <- (new Penis)@A.testAgain(45, true, "hello");
			true;
		}
	};

};
