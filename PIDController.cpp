//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------

#include "PIDController.h"

//------------------------------------------------------------------------------
// Constructors
//------------------------------------------------------------------------------

// Default constructor
PIDController::PIDController() {
    this->setGains(0, 0, 0);
    this->setInputLimits(-1, -1);
    this->setOutputLimits(-1, -1);
    this->reset();
}

// Just gains, no limits
PIDController::PIDController(double kp, double ki, double kd) {
    this->setGains(kp, ki, kd);
    this->setInputLimits(-1, -1);
    this->setOutputLimits(-1, -1);
    this->reset();
}
// Gains and output limits
PIDController::PIDController(double kp, double ki, double kd, double lowerOutputLimit, double upperOutputLimit) {
    this->setGains(kp, ki, kd);
    this->setInputLimits(-1, -1);
    this->setOutputLimits(lowerOutputLimit, upperOutputLimit);
    this->reset();
}

// All gains and limits
PIDController::PIDController(double kp, double ki, double kd, double lowerInputLimit, double upperInputLimit, double lowerOutputLimit, double upperOutputLimit) {
    this->setGains(kp, ki, kd);
    this->setInputLimits(lowerInputLimit, upperInputLimit);
    this->setOutputLimits(lowerOutputLimit, upperOutputLimit);
    this->reset();
}

// Copy constructor
PIDController::PIDController(const PIDController& orig) {
    this->setGains(orig.kp, orig.ki, orig.kd);
    this->setInputLimits(orig.lowerInputLimit, orig.upperInputLimit);
    this->setOutputLimits(orig.lowerOutputLimit, orig.lowerInputLimit);
    integrator = orig.integrator;
    lastSetpoint = orig.lastSetpoint;
    peakTime = orig.peakTime;
    settlingTime = orig.settlingTime;
    percentOvershoot = orig.percentOvershoot;
}

// Destructor
PIDController::~PIDController() {
}

//------------------------------------------------------------------------------
// Accessors
//------------------------------------------------------------------------------

double PIDController::getSetpoint() {
    return setpoint;
}

double PIDController::getKp() {
    return kp;
}

double PIDController::getKi() {
    return ki;
}

double PIDController::getKd() {
    return kd;
}

//------------------------------------------------------------------------------
// Mutators
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// targetSetpoint
//------------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : setpoint
//
// This function sets the desired setpoint that the PID controller will
// attempt to track.
//------------------------------------------------------------------------------

void PIDController::targetSetpoint(double setpoint) {
    this->setpoint = limiter(setpoint, lowerInputLimit, upperInputLimit);
    peakTime = -1;
    settlingTime = -1;
    percentOvershoot = 0;
    sample_timer.start();
    performance_timer.start();
}

//------------------------------------------------------------------------------
// setGains
//------------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : kp, ki, kd
//
// This function sets the control gains of the PID controller in order to
// achieve the desired performance characteristics.
//------------------------------------------------------------------------------

void PIDController::setGains(double kp, double ki, double kd) {
    this->kp = kp;
    this->ki = ki;
    this->kd = kd;
}

//------------------------------------------------------------------------------
// setInputLimits
//------------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : lowerInputLimit, upperInputLimit
//
// This function allows the user to set bounds on the setpoint to prevent
// undesirable behavior of the system.
//------------------------------------------------------------------------------

void PIDController::setInputLimits(double lowerInputLimit, double upperInputLimit) {
    this->lowerInputLimit = lowerInputLimit;
    this->upperInputLimit = upperInputLimit;
}

//------------------------------------------------------------------------------
// setOutputLimits
//------------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : lowerOutputLimit, upperOutputLimit
//
// This function allows the user to set bounds on the control variable to
// prevent undesirable behavior of the system.
//------------------------------------------------------------------------------

void PIDController::setOutputLimits(double lowerOutputLimit, double upperOutputLimit) {
    this->lowerOutputLimit = lowerOutputLimit;
    this->upperOutputLimit = upperOutputLimit;
}

//------------------------------------------------------------------------------
// Other Functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// limiter
//------------------------------------------------------------------------------
//
// Return Value : double
// Parameters   : value, lowerLimit, upperLimit
//
// This function compares the input 'value' to the specified limits. If the
// value exceeds the limits, the value is capped to one of the limits.
//------------------------------------------------------------------------------

double PIDController::limiter(double value, double lowerLimit, double upperLimit) {
    if (lowerLimit == upperLimit) {
        return value;
    }
    else if(value < lowerLimit) {
        return lowerLimit;
    }
    else if(value > upperLimit) {
        return upperLimit;
    }
    else
        return value;
}

//------------------------------------------------------------------------------
// reset
//------------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function initializes the PID controller to the default values.
//------------------------------------------------------------------------------

void PIDController::reset() {
    setpoint = 0;
    lastSetpoint = 0;
    integrator = 0;
    peakTime = -1;
    settlingTime = -1;
    percentOvershoot = 0;
    sample_timer.stop();
    performance_timer.stop();   
}

//------------------------------------------------------------------------------
// hasSettled
//------------------------------------------------------------------------------
//
// Return Value : bool
// Parameters   : None
//
// This function returns true only when the PID controller has stabilized to
// within 5% of its final value.
//------------------------------------------------------------------------------

bool PIDController::hasSettled() {
    if(settlingTime != -1) {
        return true;
    }
    
    else {
        return false;
    }
}

//------------------------------------------------------------------------------
// calc
//------------------------------------------------------------------------------
//
// Return Value : double
// Parameters   : processVariable
//
// This function calculates the next output value of the PID controller, given
// the current setpoint, elasped time, and feedback (processVariable). It also
// keeps track of peak time, settling time, and percent overshoot for quickly
// assessing the current performance of the controller.
//------------------------------------------------------------------------------

double PIDController::calc(double processVariable) {
    sample_timer.stop();
    
    double percent = (processVariable/setpoint) - 1;
    
    if(percent > percentOvershoot && percent > 0)
    {
        percentOvershoot = processVariable/setpoint;
        peakTime = (performance_timer.elapsed().wall)/1e9;
    }
    if(abs(percent) < 0.05 && settlingTime == -1)
    {
        performance_timer.stop();
        settlingTime = (performance_timer.elapsed().wall)/1e9;
        std::cout << "Peak Time Tp: " << peakTime << std::endl;
        std::cout << "Percent Overshoot \%OS: " << percentOvershoot << std::endl;
        std::cout << "Settling Time Ts" << settlingTime << std::endl;
    }
    
    double error = setpoint - processVariable;
    double samplingTime = (sample_timer.elapsed().wall)/1e9;
    double differentiator = (setpoint - lastSetpoint)/samplingTime;
    integrator += (error * samplingTime);
    integrator = limiter(integrator, lowerOutputLimit, upperOutputLimit);
    double controlVariable = kp * error + ki * integrator - kd * differentiator;
    
    controlVariable = limiter(controlVariable, lowerOutputLimit, upperOutputLimit);

    lastSetpoint = setpoint;
    sample_timer.start();

    return controlVariable;
}
