int zsy_test(int cond)
{
	int a;
START:
	a = 0;
	a = a + 1;
L1:
	a = a + 2;
	if (cond < 10)
		goto L2;
	else
		goto L3;
L2:
	a = a + 3;
	if (cond < 20)
		goto L4;
	else
		goto L5;
L3:
	if (cond < 30)
		goto L5;
	else
		goto L7;
L4:
	a = a + 4;
	goto L6;
L5:
	a = a + 5;
	goto L6;
L6:
	a = a + 7;
	goto L7;
L7:
	a = a + 8;
	goto STOP;

STOP:
	return a;

}
