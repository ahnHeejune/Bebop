
#include <math.h>
#include <iostream>
#include "PID.h"

// Constructor
PID::PID(float _kp, float _ki, float _kd, float _dt, float hz)
{
	Kp = _kp;
  Ki = _ki;
  Kd = _kd;
  dt = _dt;  // should be postive
	inputFilterHz = hz;
	reset();
}

/* reset status values for reuse */
void  PID::reset()
{
   integrator = 0.0f;
   lastError   = 0.0f;
}

/* input and get  PID control  output */ 
float  PID::process(float input)
{
    // P
    float error  = (target - input);

    // I 
    integrator += error*dt;
    // @TODO limit the integrator values not too large  

    // D 
	  float derivate  =  error - lastError;
    lastError = error;

    // PID 
    return Kp*error + Ki*integrator +  (Kd * derivate/dt);   // Kp* differator - error
    
}

/* set new target value */
void PID::setTarget(float _target)
{
    reset();
	  target = _target;
}



#if 0 
int main()
{
	PID pid(100.0, 100.0, 0.0, 100.0, 0.025);

    pid.setTarget(1.0);

	float  in = 0.0;

	for (int i = 0; i < 200; i++)
	{
		std::cout  << in << ":" << pid.process(in) << std::endl;
		in += 0.01;
	}

}
#endif
