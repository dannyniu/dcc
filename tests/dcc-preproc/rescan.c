#define m1(a,b) a(b)
#define m2(x) x

int x = m1(m2,m1)(0);
int y = m2(m1)(m2,1);

#define n1(x) x + x + n2
#define n2 n1(12)
int u = n1(m1(m2,n2));
