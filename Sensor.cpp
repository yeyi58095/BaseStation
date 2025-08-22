#include "Sensor.h"
#include <System.SysUtils.hpp> // IntToStr / FloatToStrF

Sensor::Sensor(int id) : id(id)
{
    // 把'成員預設值'放這裡（bcc32 不吃 { } 內初始化）
    max_queue_size = 2147483647;          // 或 <climits> 的 INT_MAX
    ITdistri = DIST_EXPONENTIAL;
	STdistri = DIST_EXPONENTIAL;
    ITpara1 = 1.0; ITpara2 = 0.0;         // λ = 1.0
	STpara1 = 1.5; STpara2 = 0.0;         // μ = 1.5   // mu is equal to lambda, in mathemetic expression,
										  // the only difference is on the statical meaning
}

void Sensor::setIT(int method, double para1, double para2)
{
    ITdistri = method;
    ITpara1  = para1;
    ITpara2  = para2;
}

void Sensor::setST(int method, double para1, double para2)
{
    STdistri = method;
    STpara1  = para1;
    STpara2  = para2;
}

AnsiString Sensor::toString() const
{
    AnsiString msg;

    msg += "Sensor: " + IntToStr(id) + "\n";

    msg += "IT: ";
    switch (ITdistri) {
        case DIST_NORMAL:
            msg += "mean=" + FloatToStrF(ITpara1, ffFixed, 7, 4)
                +  ", stddev=" + FloatToStrF(ITpara2, ffFixed, 7, 4);
            break;
        case DIST_EXPONENTIAL:
            msg += "lambda=" + FloatToStrF(ITpara1, ffFixed, 7, 4);
            break;
        case DIST_UNIFORM:
            msg += "a=" + FloatToStrF(ITpara1, ffFixed, 7, 4)
                +  ", b=" + FloatToStrF(ITpara2, ffFixed, 7, 4);
            break;
        default:
            msg += "(unknown)";
            break;
    }

    msg += "\nST: ";
    switch (STdistri) {
        case DIST_NORMAL:
            msg += "mean=" + FloatToStrF(STpara1, ffFixed, 7, 4)
                +  ", stddev=" + FloatToStrF(STpara2, ffFixed, 7, 4);
            break;
        case DIST_EXPONENTIAL:
            msg += "mu=" + FloatToStrF(STpara1, ffFixed, 7, 4);
            break;
        case DIST_UNIFORM:
            msg += "a=" + FloatToStrF(STpara1, ffFixed, 7, 4)
                +  ", b=" + FloatToStrF(STpara2, ffFixed, 7, 4);
            break;
        default:
            msg += "(unknown)";
            break;
    }
	msg += "\n";
	return msg;
}

AnsiString Sensor::dict(int a){
	switch(a){
		case 0:
			return "normal";

		case 1:
			return "exponential";

		case 2:
			return "uniform";
		default:
			return "unkown";
	}
}
