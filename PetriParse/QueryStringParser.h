#ifndef QUERYSTRINGPARSER_H
#define	QUERYSTRINGPARSER_H

#include "QueryXMLParser.h"
#include "../PetriEngine/PetriNet.h"
#include "PNMLParser.h"

#include <string>
#include <vector>

class QueryStringParser {
    public:
        QueryStringParser(QueryXMLParser *Parser, PetriEngine::PetriNet *PetriNet, PNMLParser::InhibitorArcList inhibarcs);
        void generateStateLabel(int i);
        void generateStateLabels();
        string getStateLabel(int i);
        std::vector<std::string> getStateLabels();

    private:
        void replaceOperator(std::string& query, const std::string& from, const std::string& to);
        void replacePlaces(std::string& query);
        void convertToComputeBoundsQuery(std::string& query);
        void convertToComputeBoundsForManyQuery(std::string& query, std::string& number);
        bool convertToBoundsQuery(std::string& query);
        void completeShortCut(std::string& query, int i);
        string  getPlaceIndexByName(const std::string placeName);
        void findDeadlockConditions(std::string& query, size_t deadlockPos);
        int inhibArc(unsigned int p, unsigned int t);
        void parseReachBound(int i, std::string &query);

        QueryXMLParser *_Parser;
        PetriEngine::PetriNet *_PetriNet;
        std::vector<std::string> _stateLabel;
        PNMLParser::InhibitorArcList _inhibArcs;
        
};
#endif	/* QUERYSTRINGPARSER_H */
