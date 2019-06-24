import math
A=["0"]
for i in range(0,16):
    for j in range(1,128):
            mask=0
            dx=i-8  
            dz=j
            x0=int(max((10*dx)/dz,-16))
            x1=int(min(10*(dx+4)/dz+1,16))
            for i3 in range(x0+16,x1+16):
                    mask|=(1<<i3)
            A.append(str(mask))

                
print("int64_t lut_projected_area[]={",','.join(A),"};")
    
