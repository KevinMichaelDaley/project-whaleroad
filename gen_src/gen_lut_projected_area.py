import math
A=[]
for i in range(0,128):
    for j in range(0,32):
        if j<32-i:
            mask=0
        else:
            mask=0xffffffff
            if i==31:
                mask^=1
            for theta_ in range(1000):
                    theta=theta_*0.001*180
                    ray_x=math.cos(math.pi*theta/180.0)
                    ray_y=math.sin(theta*math.pi/180.0)
                    hit=False
                    for t_ in range(100):
                        t=t_*40/100.0
                        if j>32 or i>32:
                            break
                        if abs(t*ray_y-j)<1 and abs(t*ray_x-i)<1:
                            k=int(t*ray_x)
                            hit=True
                    if hit and (mask&(1<<int(k)) !=0):
                        mask^=(1<<int(k))
        A.append(str(mask))

                
print("uint32_t lut_projected_area[]={",','.join(A),"};")
