
#include "Master.h"
#include <vector>
#include <stdlib.h>
#include <time.h>

// �A�{���� Sensor ���O
class Sensor;

// === �A�ݭn�Ρu�ۤv�{���� GUI �ͦ��{���X�v��@�o�Ӥu�t ===
//   �� GUI �W�쥻�Ψӫإ�/��l�ƨC�� Sensor�]�f, �g, e, C ���^���q�h�i�ӡC
//   �`�N�G���n�Υ��� VCL/TeeChart/��檫��C
//   �U�������@�ӪŴߡA����sĶ�z���A�A�n�� TODO �����u���C
static void CreateSensorsForHeadless(double lambda, double mu, double e, int C,
                                     unsigned int seed,
                                     std::vector<Sensor*>& out)
{
	// TODO: �̧A���]�p push_back N �� Sensor*
	//       �ó]�w���̪���F�v/�A�Ȳv/EP �Ѽ�/�e�q C ���C
	// �d�ҡ]���] 1 �� sensor�F�A�i�令�h�ӡ^�G
	// out.push_back(new Sensor(lambda, mu, e, C, seed));
	(void)lambda; (void)mu; (void)e; (void)C; (void)seed;
}

extern "C" int RunSimulationCore(
	double mu, double e, int C, double lambda, int T, unsigned int seed,
	double* avg_delay_ms, double* L, double* W, double* loss_rate, double* EP_mean)
{
	if (!avg_delay_ms || !L || !W || !loss_rate || !EP_mean) return -10;
	if (seed == 0) seed = (unsigned int)time(NULL);

	// 1) �إ� sensors�]�ΧA�쥻���޿�^
	std::vector<Sensor*> sensors;
	CreateSensorsForHeadless(lambda, mu, e, C, seed, sensors);

	// 2) �إ� Master�B�������t�������A�]�w�����ɶ�
	sim::Master m;
	m.setSensors(&sensors);
	m.setEndTime((double)T);

	// �@�Ǳ`�Ϊ��uheadless �M�n�v�]�w�]�i�̧A�ݨD�վ�^
	m.logMode           = sim::Master::LOG_NONE;
	m.recordTrace       = false;
	m.logStateEachEvent = false;
	m.alwaysCharge      = false;        // �̧A������
	m.setChargingSlots(0);              // 0=unlimited�]�Y�A���h�u�R�q�^

	// 3) �]����
	m.reset();
	m.run();

	// 4) �� KPI
	sim::Master::KPIs kpi;
	if (!m.computeKPIs(kpi)) {
		// �M�z sensors
		int i;
		for (i = 0; i < (int)sensors.size(); ++i) delete sensors[i];
		sensors.clear();
		return -20;
	}

	// 5) ��^�I�s�̻ݭn���έp
	*avg_delay_ms = kpi.avg_delay_ms;
	*L            = kpi.L;
	*W            = kpi.W;
	*loss_rate    = kpi.loss_rate;
	*EP_mean      = kpi.EP_mean;

	// 6) �M�z sensors�]�קK�O����~���^
	int i;
	for (i = 0; i < (int)sensors.size(); ++i) delete sensors[i];
	sensors.clear();

	return 0;
}
