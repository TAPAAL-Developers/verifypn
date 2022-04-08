#include "CTL/CTLResult.h"
#include <iomanip>

void CTLResult::print(const std::string& qname, StatisticsLevel statisticslevel, size_t index, options_t& options, std::ostream& out) const {

    const static std::string techniques = "TECHNIQUES COLLATERAL_PROCESSING EXPLICIT STATE_COMPRESSION SAT_SMT ";

    out << "\n";
    out << "FORMULA "
         << qname
         << " " << (result ? "TRUE" : "FALSE") << " "
         << techniques
         << (options.isCPN ? "UNFOLDING_TO_PT " : "")
         << (options.stubbornreduction ? "STUBBORN_SETS " : "")
         << (options.ctlalgorithm == CTL::CZero ? "CTL_CZERO " : "")
         << (options.ctlalgorithm == CTL::Local ? "CTL_LOCAL " : "")
// FIXME debugging print
#ifdef DG_SOURCE_CHECK
         << "SOURCE_CHECK "
#endif
#ifdef DG_LAZY_CHECK
         << "LAZY_CHECK "
#endif
#ifdef DG_REFCOUNTING
         << "REFCOUNTING "
#endif
            << "\n\n";

    out << "Query index " << index << " was solved" << "\n";
    out << "Query is" << (result ? "" : " NOT") << " satisfied." << "\n";

    if(statisticslevel != StatisticsLevel::None){
        out << "\n";
        out << "STATS:" << "\n";
        out << "	Time (seconds)    : " << std::setprecision(4) << duration / 1000 << "\n";
        out << "	Configurations    : " << numberOfConfigurations << "\n";
        out << "	Markings          : " << numberOfMarkings << "\n";
        out << "	Edges             : " << numberOfEdges << "\n";
        out << "	Processed Edges   : " << processedEdges << "\n";
        out << "	Processed N. Edges: " << processedNegationEdges << "\n";
        out << "	Explored Configs  : " << exploredConfigurations << "\n";
        if (optimProcs > 0)
          std::cerr << "	Dependent Clears  : " << optimProcs << "\n";
        else
          out << "	Dependent Clears  : " << optimProcs << "\n";
    }
    out << std::endl;
}
