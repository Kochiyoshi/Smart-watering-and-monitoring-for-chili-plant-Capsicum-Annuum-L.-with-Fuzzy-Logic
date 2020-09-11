//***********************************************************************
// Author : Arief Maulana                  
// NIM    : 1301144158                                             
// Email  : ariefmaulana@students.telkomuniversity.ac.id
//          arief.thehunter34@gmail.com                                                
// Matlab .fis to arduino C converter v2.0.1.25122016                      
//***********************************************************************

#include "fis_header.h"
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

#define WATERPIN3 3 //pin digital 3 use for water pump

SoftwareSerial s(5,6); //pin digital 5,6 use for serial connection with NodeMCU

const int fis_gcI = 2; // Number of inputs to the fuzzy inference system
const int fis_gcO = 1; // Number of outputs to the fuzzy inference system
const int fis_gcR = 9; // Number of rules to the fuzzy inference system
FIS_TYPE g_fisInput[fis_gcI];
FIS_TYPE g_fisOutput[fis_gcO];

//variable to contain raw data directly from sensor
float sensor_value_PH; //pH sensor
float sensor_value_TEMP; //Temprature sensor
float sensor_value_MOISTURE; //Moisture sensor

//variable to contain proccess data from sensor
float output_value_PH;
float output_value_degreeC_TEMP; //Temprature sensor in Celcius
float output_value_degreeF_TEMP; //Temprature sensor in Fahrenheit
float output_value_MOISTURE; 

int check = 0; //variable for time check using delay
int fuzzy_status = 0; //variable to contain data for how much water will be use for watering

// Setup routine runs once when you press reset:
void setup()
{
    Serial.begin(115200);    
    s.begin(115200);
                                        
    // initialize the Analog pins for input.    
    pinMode(A1 , INPUT);  // Analog pin mode Input: pH    
    pinMode(A3 , INPUT);  // Analog pin mode Input: Temperature    
    pinMode(A0 , INPUT);  // Analog pin mode Input: Soil_Moisture    
    pinMode(3, OUTPUT);   // Digital pin mode Output: Water Pump
    delay(1000);   
}

StaticJsonBuffer<500> jsonBuffer;
JsonObject& root = jsonBuffer.createObject();

// Loop routine runs over and over again forever:
void loop()
{
    //read raw data from sensors
    sensor_value_PH = analogRead(A1);
    sensor_value_TEMP = analogRead(A3) * 0.004882814;
    sensor_value_MOISTURE = analogRead(A0);

    //Processing raw datas to get real world value
    output_value_PH = (-0.062657232*sensor_value_PH)+11.16264704;
    output_value_degreeC_TEMP = (sensor_value_TEMP - 0.5) * 100.0;
    output_value_degreeF_TEMP = output_value_degreeC_TEMP * (9.0/5.0) + 32.0;
    output_value_MOISTURE = map(sensor_value_MOISTURE,603,240,0,100);
    
    //fuzzy data input
    g_fisInput[0] = output_value_degreeC_TEMP; // Read Input : Temperature
    g_fisInput[1] = output_value_MOISTURE; // Read Input : Soil_Moisture
    g_fisOutput[0] = 0; 
    fis_evaluate(); 

    Serial.print("Ph: ");
    Serial.println(output_value_PH);
    Serial.print("Moisture: ");
    Serial.println(output_value_MOISTURE);
    Serial.print("deg C: ");
    Serial.println(output_value_degreeC_TEMP);
    Serial.println(g_fisOutput[0]);

    check=(check+60); //use for time referance in 60 sec
    fuzzy_status=0;

    Serial.print(check);
    Serial.print(" in second   ");
    Serial.print(check/60);
    Serial.print( " in minute   ");
    Serial.print(check/3600 );
    Serial.println( " in hour" );
    Serial.println("");

    // Use for watering depend on fuzzy value
    if (check>=10800){                                            //for every 10800sec (3 hours) plant will be water if it need to
    //if (check>=180){                                            //for every 180sec (3 minutes) plant will be water if it need to (FOR DEBUGING)
      if (g_fisOutput[0] <=10){
        fuzzy_status=3;                                           //fuzzy status : "No Watering"
      }else if (g_fisOutput[0] >10 && g_fisOutput[0]<=20){
        fuzzy_status=1;                                           //fuzzy status : "Medium Watering"
        digitalWrite(WATERPIN3, 200);
        delay(3000);
        digitalWrite(WATERPIN3, LOW);
      }else if (g_fisOutput[0] >20){
        fuzzy_status=2;                                            //fuzzy status : "High Watering"
        digitalWrite(WATERPIN3, 200);
        delay(10000);
        digitalWrite(WATERPIN3, LOW);
      }
      check=0;
    }

    //For arduino sends data to Node MCU
    if (isnan(output_value_PH) || isnan(output_value_MOISTURE) || isnan(output_value_degreeC_TEMP) || isnan(g_fisOutput[0]) || isnan(fuzzy_status) ) {
    return;
    }
    // If the sensor is not connected to correct pin or if it doesnot
    //work no data will be sent
    root["ph"] = output_value_PH;
    root["mois"] = output_value_MOISTURE;
    root["c"] = output_value_degreeC_TEMP;
    root["f"] = g_fisOutput[0];
    root["f_s"] = fuzzy_status;
    //root["ph_s"] = ph_status;
 
    if(s.available()>0)
    {
      root.printTo(s);
    }

        
    delay(60000); 
}


//***********************************************************************
// Support functions for Fuzzy Inference System                          
//***********************************************************************
// Trapezoidal Member Function
FIS_TYPE fis_trapmf(FIS_TYPE x, FIS_TYPE* p)
{
    FIS_TYPE a = p[0], b = p[1], c = p[2], d = p[3];
    FIS_TYPE t1 = ((x <= c) ? 1 : ((d < x) ? 0 : ((c != d) ? ((d - x) / (d - c)) : 0)));
    FIS_TYPE t2 = ((b <= x) ? 1 : ((x < a) ? 0 : ((a != b) ? ((x - a) / (b - a)) : 0)));
    return (FIS_TYPE) min(t1, t2);
}

// Triangular Member Function
FIS_TYPE fis_trimf(FIS_TYPE x, FIS_TYPE* p)
{
    FIS_TYPE a = p[0], b = p[1], c = p[2];
    FIS_TYPE t1 = (x - a) / (b - a);
    FIS_TYPE t2 = (c - x) / (c - b);
    if ((a == b) && (b == c)) return (FIS_TYPE) (x == a);
    if (a == b) return (FIS_TYPE) (t2*(b <= x)*(x <= c));
    if (b == c) return (FIS_TYPE) (t1*(a <= x)*(x <= b));
    t1 = min(t1, t2);
    return (FIS_TYPE) max(t1, 0);
}

FIS_TYPE fis_min(FIS_TYPE a, FIS_TYPE b)
{
    return min(a, b);
}

FIS_TYPE fis_max(FIS_TYPE a, FIS_TYPE b)
{
    return max(a, b);
}

FIS_TYPE fis_array_operation(FIS_TYPE *array, int size, _FIS_ARR_OP pfnOp)
{
    int i;
    FIS_TYPE ret = 0;

    if (size == 0) return ret;
    if (size == 1) return array[0];

    ret = array[0];
    for (i = 1; i < size; i++)
    {
        ret = (*pfnOp)(ret, array[i]);
    }

    return ret;
}


//***********************************************************************
// Data for Fuzzy Inference System                                       
//***********************************************************************
// Pointers to the implementations of member functions
_FIS_MF fis_gMF[] =
{
    fis_trapmf, fis_trimf
};

// Count of member function for each Input
int fis_gIMFCount[] = { 3, 3 };

// Count of member function for each Output 
int fis_gOMFCount[] = { 3 };

// Coefficients for the Input Member Functions
FIS_TYPE fis_gMFI0Coeff1[] = { -100, -100, 13, 19 };
FIS_TYPE fis_gMFI0Coeff2[] = { 13, 19.75, 30, 36 };
FIS_TYPE fis_gMFI0Coeff3[] = { 30, 36, 99.9, 99.9 };
FIS_TYPE* fis_gMFI0Coeff[] = { fis_gMFI0Coeff1, fis_gMFI0Coeff2, fis_gMFI0Coeff3 };
FIS_TYPE fis_gMFI1Coeff1[] = { -100, -100, 30, 50 };
FIS_TYPE fis_gMFI1Coeff2[] = { 30, 50, 70 };
FIS_TYPE fis_gMFI1Coeff3[] = { 50, 70, 100, 100 };
FIS_TYPE* fis_gMFI1Coeff[] = { fis_gMFI1Coeff1, fis_gMFI1Coeff2, fis_gMFI1Coeff3 };
FIS_TYPE** fis_gMFICoeff[] = { fis_gMFI0Coeff, fis_gMFI1Coeff };

// Coefficients for the Output Member Functions
FIS_TYPE fis_gMFO0Coeff1[] = { 0, 5, 10 };
FIS_TYPE fis_gMFO0Coeff2[] = { 10, 15, 20 };
FIS_TYPE fis_gMFO0Coeff3[] = { 20, 25, 30 };
FIS_TYPE* fis_gMFO0Coeff[] = { fis_gMFO0Coeff1, fis_gMFO0Coeff2, fis_gMFO0Coeff3 };
FIS_TYPE** fis_gMFOCoeff[] = { fis_gMFO0Coeff };

// Input membership function set
int fis_gMFI0[] = { 0, 0, 0 };
int fis_gMFI1[] = { 0, 1, 0 };
int* fis_gMFI[] = { fis_gMFI0, fis_gMFI1};

// Output membership function set
int fis_gMFO0[] = { 1, 1, 1 };
int* fis_gMFO[] = { fis_gMFO0};

// Rule Weights
FIS_TYPE fis_gRWeight[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };

// Rule Type
int fis_gRType[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };

// Rule Inputs
int fis_gRI0[] = { 1, 1 };
int fis_gRI1[] = { 1, 2 };
int fis_gRI2[] = { 2, 1 };
int fis_gRI3[] = { 2, 2 };
int fis_gRI4[] = { 2, 3 };
int fis_gRI5[] = { 3, 1 };
int fis_gRI6[] = { 3, 2 };
int fis_gRI7[] = { 3, 3 };
int fis_gRI8[] = { 1, 3 };
int* fis_gRI[] = { fis_gRI0, fis_gRI1, fis_gRI2, fis_gRI3, fis_gRI4, fis_gRI5, fis_gRI6, fis_gRI7, fis_gRI8 };

// Rule Outputs
int fis_gRO0[] = { 2 };
int fis_gRO1[] = { 1 };
int fis_gRO2[] = { 3 };
int fis_gRO3[] = { 2 };
int fis_gRO4[] = { 1 };
int fis_gRO5[] = { 3 };
int fis_gRO6[] = { 3 };
int fis_gRO7[] = { 1 };
int fis_gRO8[] = { 1 };
int* fis_gRO[] = { fis_gRO0, fis_gRO1, fis_gRO2, fis_gRO3, fis_gRO4, fis_gRO5, fis_gRO6, fis_gRO7, fis_gRO8 };

// Input range Min
FIS_TYPE fis_gIMin[] = { 0, 0 };

// Input range Max
FIS_TYPE fis_gIMax[] = { 47, 100 };

// Output range Min
FIS_TYPE fis_gOMin[] = { 0 };

// Output range Max
FIS_TYPE fis_gOMax[] = { 30 };

//***********************************************************************
// Data dependent support functions for Fuzzy Inference System           
//***********************************************************************
FIS_TYPE fis_MF_out(FIS_TYPE** fuzzyRuleSet, FIS_TYPE x, int o)
{
    FIS_TYPE mfOut;
    int r;

    for (r = 0; r < fis_gcR; ++r)
    {
        int index = fis_gRO[r][o];
        if (index > 0)
        {
            index = index - 1;
            mfOut = (fis_gMF[fis_gMFO[o][index]])(x, fis_gMFOCoeff[o][index]);
        }
        else if (index < 0)
        {
            index = -index - 1;
            mfOut = 1 - (fis_gMF[fis_gMFO[o][index]])(x, fis_gMFOCoeff[o][index]);
        }
        else
        {
            mfOut = 0;
        }

        fuzzyRuleSet[0][r] = fis_min(mfOut, fuzzyRuleSet[1][r]);
    }
    return fis_array_operation(fuzzyRuleSet[0], fis_gcR, fis_max);
}

FIS_TYPE fis_defuzz_centroid(FIS_TYPE** fuzzyRuleSet, int o)
{
    FIS_TYPE step = (fis_gOMax[o] - fis_gOMin[o]) / (FIS_RESOLUSION - 1);
    FIS_TYPE area = 0;
    FIS_TYPE momentum = 0;
    FIS_TYPE dist, slice;
    int i;

    // calculate the area under the curve formed by the MF outputs
    for (i = 0; i < FIS_RESOLUSION; ++i){
        dist = fis_gOMin[o] + (step * i);
        slice = step * fis_MF_out(fuzzyRuleSet, dist, o);
        area += slice;
        momentum += slice*dist;
    }

    return ((area == 0) ? ((fis_gOMax[o] + fis_gOMin[o]) / 2) : (momentum / area));
}

//***********************************************************************
// Fuzzy Inference System                                                
//***********************************************************************
void fis_evaluate()
{
    FIS_TYPE fuzzyInput0[] = { 0, 0, 0 };
    FIS_TYPE fuzzyInput1[] = { 0, 0, 0 };
    FIS_TYPE* fuzzyInput[fis_gcI] = { fuzzyInput0, fuzzyInput1, };
    FIS_TYPE fuzzyOutput0[] = { 0, 0, 0 };
    FIS_TYPE* fuzzyOutput[fis_gcO] = { fuzzyOutput0, };
    FIS_TYPE fuzzyRules[fis_gcR] = { 0 };
    FIS_TYPE fuzzyFires[fis_gcR] = { 0 };
    FIS_TYPE* fuzzyRuleSet[] = { fuzzyRules, fuzzyFires };
    FIS_TYPE sW = 0;

    // Transforming input to fuzzy Input
    int i, j, r, o;
    for (i = 0; i < fis_gcI; ++i)
    {
        for (j = 0; j < fis_gIMFCount[i]; ++j)
        {
            fuzzyInput[i][j] =
                (fis_gMF[fis_gMFI[i][j]])(g_fisInput[i], fis_gMFICoeff[i][j]);
        }
    }

    int index = 0;
    for (r = 0; r < fis_gcR; ++r)
    {
        if (fis_gRType[r] == 1)
        {
            fuzzyFires[r] = FIS_MAX;
            for (i = 0; i < fis_gcI; ++i)
            {
                index = fis_gRI[r][i];
                if (index > 0)
                    fuzzyFires[r] = fis_min(fuzzyFires[r], fuzzyInput[i][index - 1]);
                else if (index < 0)
                    fuzzyFires[r] = fis_min(fuzzyFires[r], 1 - fuzzyInput[i][-index - 1]);
                else
                    fuzzyFires[r] = fis_min(fuzzyFires[r], 1);
            }
        }
        else
        {
            fuzzyFires[r] = FIS_MIN;
            for (i = 0; i < fis_gcI; ++i)
            {
                index = fis_gRI[r][i];
                if (index > 0)
                    fuzzyFires[r] = fis_max(fuzzyFires[r], fuzzyInput[i][index - 1]);
                else if (index < 0)
                    fuzzyFires[r] = fis_max(fuzzyFires[r], 1 - fuzzyInput[i][-index - 1]);
                else
                    fuzzyFires[r] = fis_max(fuzzyFires[r], 0);
            }
        }

        fuzzyFires[r] = fis_gRWeight[r] * fuzzyFires[r];
        sW += fuzzyFires[r];
    }

    if (sW == 0)
    {
        for (o = 0; o < fis_gcO; ++o)
        {
            g_fisOutput[o] = ((fis_gOMax[o] + fis_gOMin[o]) / 2);
        }
    }
    else
    {
        for (o = 0; o < fis_gcO; ++o)
        {
            g_fisOutput[o] = fis_defuzz_centroid(fuzzyRuleSet, o);
        }
    }
}
