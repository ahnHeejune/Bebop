#ifndef PID_H
#define PID_H

class PID {
public:

  // Constructor for PID
  PID(float _kp, float _ki, float _kd, float _dt, float inputFilterHz = 1.0);
	void  setTarget(float _target);
	float process(float input);
	void  reset();
  float getTarget()
  {
			return target;
  }
  float getError()
	{	
      return lastError;
	}
  float getIntegrator()
	{	
      return integrator;
  }
    
protected:

    // parameters
    float	Kp;
    float	Ki;
    float	Kd;
    float	dt;                    // timestep in seconds
    
	  float	inputFilterHz;                   // PID Input filter frequency in Hz
	  float	target;

    float   integrator;            // integrator value
    float   lastInput;             // last input for derivative
    float   lastError;             // 

};

#endif // PID_H
