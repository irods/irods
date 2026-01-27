test{
	*a="hello/world/";
	for(*i=0;*i<1000;*i=*i+1) {
		test2(*a,*b,*c);
	}
}

test2(*a,*b,*c){
	msiSplitPath(*a,*b,*c);
}
input null
output null


