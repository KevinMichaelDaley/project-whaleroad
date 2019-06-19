in vec4 p;
layout(location=0) out float color;
	void main()
	{
		float depth = p.z / p.w ;
		depth = depth * 0.5 + 0.5;			//Don't forget to move away from unit cube ([-1,1]) to [0,1] coordinate system
	
        color=depth;
    }	
