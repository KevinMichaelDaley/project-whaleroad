in vec4 p;
layout(location=0) out float color;

	void main()
	{
		color = p.z / p.w*0.5+0.5 ;
	
		// Adjusting moments (this is sort of bias per pixel) using partial derivative
		
		
    }	
