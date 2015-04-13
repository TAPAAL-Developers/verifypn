#ifndef QUERYSTRINGPARSER_H
#define	QUERYSTRINGPARSER_H

#include "QueryXMLParser.h"
#include "../PetriEngine/PetriNet.h"

#include <string>
#include <vector>

class QueryStringParser {
    public:
        QueryStringParser(QueryXMLParser *Parser, PetriEngine::PetriNet *PetriNet);
        void generateStateLabel(int i);
        void generateStateLabels();
        string getStateLabel(int i);
        std::vector<std::string> getStateLabels();

        void replaceOperator(std::string& query, const std::string& from, const std::string& to);
        void replacePlaces(std::string& query);
        string  getPlaceIndexByName(const std::string placeName);

    private:
        QueryXMLParser *_Parser;
        PetriEngine::PetriNet *_PetriNet;
        std::vector<std::string> _stateLabel;
};
#endif	/* QUERYSTRINGPARSER_H */
