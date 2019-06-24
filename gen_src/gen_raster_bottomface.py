from sympy import *

M=Matrix([[-145.900223,0.000000,0.000000,0.000000],[0.000000,145.900223,0.000000,0.000000],[0.000000,0.000000,1.020202,-0.202020],[0.000000,0.000000,1.000000,0.000000]])
x=Symbol('x')
y=Symbol('y')
z=Symbol('z')
X=Matrix([[x],[y],[z],[1]])
X2=Matrix([[x+1],[y],[z],[1]])
X3=Matrix([[x],[y],[z],[1]])
X4=Matrix([[x],[y+1],[z],[1]])
Y=M*X/(M*X)[3]
print(pretty(Y))
s=[]
for i in range(1,128):
    s.append(str(int(expand(64.0/(Y[2]**8)).subs(z,i).evalf())))
print("int form_factor[]={"+",".join(s)+"};")
