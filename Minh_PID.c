#pragma config(Sensor, in1,    gyro,           sensorGyro)
#pragma config(Motor,  port2,           leftDrive,     tmotorVex393TurboSpeed_MC29, openLoop)
#pragma config(Motor,  port3,           armLift_Left,  tmotorVex393TurboSpeed_MC29, openLoop)
#pragma config(Motor,  port4,           forkLift_Left, tmotorVex393TurboSpeed_MC29, openLoop)
#pragma config(Motor,  port7,           forkLift_Right, tmotorVex393TurboSpeed_MC29, openLoop, reversed)
#pragma config(Motor,  port8,           armLift_Right, tmotorVex393TurboSpeed_MC29, openLoop, reversed)
#pragma config(Motor,  port9,           rightDrive,    tmotorVex393TurboSpeed_MC29, openLoop, reversed)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//
// assume that one motor for each function, on each side
// encoder on left sides except on drive where there's one on each side
float distance_per_tile = 1;

float kP_Drive = 0;
float kI_Drive = 0;
float kD_Drive = 0;

float kP_driftCorrection = 0;
float kI_driftCorrection = 0;
float kD_driftCorrection = 0;

float kP_Rotate = 0;
float kI_Rotate = 0;
float kD_Rotate = 0;

float kP_Arm = 0;
float kI_Arm = 0;
float kD_Arm = 0;

float kP_Fork = 0;
float kI_Fork = 0;
float kD_Fork = 0;

// ARM TASK
float armTarget = 0;
float armError = 0;
float arm_Pasr_Error= 0;
float armDerivative = 0;
float armIntegral = 0;
float arm_Integral_Active_Range = 100;
float maxRange = 1;
float armScale = (127/abs(maxRange));

// FORK TASK
float forkTarget = 0;
float forkError = 0;
float fork_Pasr_Error= 0;
float forkDerivative = 0;
float forkIntegral = 0;
float fork_Integral_Active_Range = 100;
float fork_maxRange = 1;
float forkScale = (127/abs(fork_maxRange));
float forkUp = 1000; //pretty much the max range
float forkDown = 0; // the starting position, assuming robot start with fork down

void initialize(){
	slaveMotor(armLift_Right, armLift_Left); // armLift_Left is master
	slaveMotor(forkLift_Right, forkLift_Left); // forkLift_Left is master
	// set encoder value to zero
	nMotorEncoder(leftDrive) = 0;
	nMotorEncoder(rightDrive) = 0;
	nMotorEncoder(armLift_Left) = 0;
	nMotorEncoder(forkLift_Left) = 0;
	wait1Msec(100);
}

void initialize_Gyro(){
	// set gyro to zero
	SensorType[gyro] = sensorNone;
	wait1Msec(1000);
	SensorType[gyro] = sensorGyro;
	wait1Msec(2000);
}

void clearEncoder(){
	nMotorEncoder(leftDrive) = 0;
	nMotorEncoder(rightDrive) = 0;
	wait1Msec(100);
	clearTimer(T1);
}

// direction is either 1 (forward) or -1 (backward)
// tile_Distance is distance in term of tile
void drive (int direction, float tile_Distance){
	clearEncoder();
	float target_Distance = direction * tile_Distance * distance_per_tile;

	// DRIVE PID VARIABLES
	float driveValue = nMotorEncoder(leftDrive);
	float driveError  = target_Distance - driveValue;
	float scaling = 120/abs(driveError); //scaling the power value by the distance needed to travel. need to cap power at 120 to leave room for DRIFT PID to take effect
	float pastError = 0;
	float driveIntegral = 0;
	float driveIntegral_Active_Range = 100;
	float driveDerivative = 0;
	float drivePower = 0;

	// DRIFT PID VARIABLES
	float reference_Angle = SensorValue(gyro);
	float drift_Scaling = 7/3600; //scaling the power value by 360 degree
	float drift_Derivative = 0;
	float drift_Integral = 0;
	float drift_Power = 0;
	float drift_Past_Error = 0;
	float drift_Integral_Active_Range = 50; // within 5 degree

	// PID LOOP
	while ((time1[T1] < 4000) && (abs(driveError) < 50)){

		// DRIVE PID CALCULATIONS
		driveValue = nMotorEncoder(leftDrive);
		driveError = target_Distance - driveValue;
		driveDerivative = driveError - pastError;
		driveIntegral = driveIntegral + driveError;
		if (abs(driveError) > driveIntegral_Active_Range){ //make sure integeral don't get too big
			driveIntegral = 0;
		}
		if (driveError == 0){ //stop the integral when there's no more error
			driveIntegral = 0;
		}
		drivePower = (kP_Drive *  driveError) + (kI_Drive * driveIntegral) + (kD_Drive * driveDerivative);

		// DRIFT PID CALCULATIONS
		float drift_Error = SensorValue(gyro) - reference_Angle; // negative = drift right, positive = drift left
		drift_Derivative = drift_Error - drift_Past_Error;
		drift_Integral = drift_Integral + drift_Error;
		if (abs(drift_Error) > drift_Integral_Active_Range){ //make sure integeral don't get too big
			drift_Integral = 0;
		}
		if (drift_Error == 0){ //stop the integral when there's no more error
			drift_Integral = 0;
		}
		drift_Power = (kP_driftCorrection *  drift_Error) + (kI_driftCorrection * drift_Integral) + (kD_driftCorrection * drift_Derivative);

		// OUTPUT
		motor[leftDrive] = (drivePower * scaling) + (drift_Power * drift_Scaling); // if drift_Power is negative => drift right => boost right, lower left
		motor[rightDrive] = (drivePower * scaling) - (drift_Power * drift_Scaling); // if drift_Power is positive => drift left => boost left, lower right

		pastError = driveError;
		drift_Past_Error = drift_Error;
		wait1Msec(15);
	}
}


// direction: 1 (counterclockwise, left) or -1 (clockwise, right)
// angle is angle in degree
void rotate (int direction, float angle){
	float gyro_Target = direction * angle * 10; // gyro value: -3600 to 3600
	float startAngle = SensorValue(gyro);
	float gyroValue = SensorValue(gyro) - startAngle;
	float gyro_Error = gyro_Target - gyroValue;
	float gyro_Past_Error = 0;
	float gyro_Derivative = 0;
	float gyro_Integral = 0;
	float gyro_Integral_Active_Range = 100; // within 10 degree
	float gyro_Scaling = (127/3600);
	clearTimer(T1);
	while ((time1[T1] < 4000) && (abs(gyro_Error) < 50)){ //5 degree
		gyroValue = SensorValue(gyro) - startAngle;
		gyro_Error = gyro_Target - gyroValue;
		gyro_Derivative = gyro_Error - gyro_Past_Error;
		gyro_Integral = gyro_Integral + gyro_Error;
		if (abs(gyro_Integral) > gyro_Integral_Active_Range){
			gyro_Integral = 0;
		}
		if (gyro_Error == 0){
			gyro_Integral = 0;
		}

		// OUTPUT
		motor[leftDrive] = (kP_Rotate *  gyro_Error) + (kI_Rotate * gyro_Integral) + (kD_Rotate * gyro_Derivative) * gyro_Scaling;
		motor[rightDrive] = (-1) * (kP_Rotate *  gyro_Error) + (kI_Rotate * gyro_Integral) + (kD_Rotate * gyro_Derivative) * gyro_Scaling;

		gyro_Past_Error = gyro_Error;
		wait1Msec(15);
	}
}


task arm(){
	armError = armTarget - nMotorEncoder(armLift_Left);
	clearTimer(T2);
	while ((time1[T2]<4000) && (abs(armError) < 50)){
		armError = armTarget - nMotorEncoder(armLift_Left);
		armDerivative = armError - arm_Pasr_Error;
		armIntegral = armIntegral + armError;
		if (abs(armIntegral) > arm_Integral_Active_Range){
			armIntegral = 0;
		}

		// OUTPUT
		motor[leftDrive] = (kP_Arm *  armError) + (kI_Arm * armIntegral) + (kD_Arm * armDerivative) * armScale;
		motor[rightDrive] = (kP_Arm *  armError) + (kI_Arm * armIntegral) + (kD_Arm * armDerivative) * armScale;

		arm_Pasr_Error = armError;
		wait1Msec(15);
	}
}


task fork(){
	forkError = forkTarget - nMotorEncoder(forkLift_Left);
	clearTimer(T3);
	while ((time1[T3]<4000) && (abs(armError) < 50)){
		forkError = forkTarget - nMotorEncoder(forkLift_Left);
		forkDerivative = forkError - fork_Pasr_Error;
		forkIntegral = forkIntegral + forkError;
		if (abs(forkIntegral) > fork_Integral_Active_Range){
			forkIntegral = 0;
		}

		// OUTPUT
		motor[leftDrive] = (kP_Fork *  forkError) + (kI_Fork * forkIntegral) + (kD_Fork * forkDerivative) * forkScale;
		motor[rightDrive] = (kP_Fork *  forkError) + (kI_Fork * forkIntegral) + (kD_Fork * forkDerivative) * forkScale;

		fork_Pasr_Error = forkError;
		wait1Msec(15);
	}

}


task main()
{
	initialize();
	initialize_Gyro();

	startTask(arm);
	startTask(fork);

	// sequence begin

}
