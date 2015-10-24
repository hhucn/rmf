/**
 * @authors Tobias Krauthoff, Sebastian Wilken
 * @date 23. June 2014
 * calculates and smoothen results
 */

#ifndef RESULTCALCULATOR_H_
#define RESULTCALCULATOR_H_

// Namespace
using namespace std;

// Includes
#include "inputHandler.h"
#include <stdint.h>
#include <fstream>

extern class inputHandler inputH;

class ResultCalculator {
	private:
		uint32_t nChirpsize_;
		uint32_t nNodeRole_;
		uint32_t nCountSmoothening_;
		string logfilePath_;

		bool isCreateLossPlot_;
		bool isCreateGapPlot_;
		bool isCreateCongestionPlot_;
		bool isCreateChirpAndPacketSizePlot_;

		int32_t openRawDataFile(fstream &);
		int32_t createProcessedDataFile(fstream &);
		int32_t createDebugDataFile(fstream &);
		int32_t creatGapPlotFile();
		int32_t createCongestionPlotFile();
		int32_t createProcessedPlotFile();
		int32_t createDebugPlotFile();
		int32_t createLossPlotFile();
		int32_t createChirpAndPacketSizePlotFile();

	public:
		ResultCalculator(uint32_t chirpsize, uint32_t noderole, uint32_t countSmoothening, string logfilepath);
		virtual ~ResultCalculator();

		int32_t calculateResults();

		void set_createLossPlot(bool createPlot){ isCreateLossPlot_ = createPlot;};
		void set_createGapPlot(bool createPlot){ isCreateGapPlot_ = createPlot;};
		void set_createCongestionPlot(bool congPlot){ isCreateCongestionPlot_ = congPlot;};
		void set_createChirpAndPacketSizePlot(bool createPlot){ isCreateChirpAndPacketSizePlot_ = createPlot;};
};

#endif /* RESULTCALCULATOR_H_ */
