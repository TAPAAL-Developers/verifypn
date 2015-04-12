#include "QueryStringParser.h"

#include <iostream>

using namespace std;

    QueryStringParser::QueryStringParser(QueryXMLParser *Parser, PetriEngine::PetriNet *PetriNet){
        _Parser = Parser;
        _PetriNet = PetriNet;

        // Create placeholders in statelabel vector
        for(std::vector<QueryXMLParser::QueryItem>::iterator it = _Parser->queries.begin();
                it != _Parser->queries.end(); ++it){
            _stateLabel.push_back("");
        }
    }

    void QueryStringParser::replaceOperator(std::string& query, const std::string& from, const std::string& to) {
        size_t startPos = 0;
        while((startPos = query.find(from, startPos)) != std::string::npos) {
            size_t endPos = from.length();
            query.replace(startPos, endPos, to);
            startPos += to.length();
        }
    }

    string QueryStringParser::getPlaceIndexByName(const std::string placeName){
        int i;
        string index;
        stringstream ss;

        const std::vector<std::string> placeNames = _PetriNet->placeNames();

        for(i = 0; i < placeNames.size(); i++){
            if(placeNames[i] == placeName){
                break;
            }
        }

        ss << i;
        index = ss.str();

        return index;
    }

    void QueryStringParser::replacePlaces(std::string& query) {
        size_t startPos = 0;

        while((startPos = query.find("\"", startPos)) != std::string::npos) {
            size_t end_quote = query.find("\"", startPos + 1);
            size_t nameLen = (end_quote - startPos) + 1;

            // Exclude both quotes from place name before searching
            string oldPlaceName = query.substr(startPos + 1, nameLen - 2);
            string newPlaceIndex = getPlaceIndexByName(oldPlaceName);
            string newPlaceName = "src[" + newPlaceIndex + "]";

            query.replace(startPos, nameLen, newPlaceName);
            startPos = end_quote;
        }
    }

    void QueryStringParser::generateStateLabel(int i){
        string query = _Parser->queries[i].queryText;

        // Replace all TAPAAL query operators with C operators
        replaceOperator(query, "not", "!");
        replaceOperator(query, "and", "&&");
        replaceOperator(query, "or", "||");

        // Rename places eg. "place0" -> src[0]
        replacePlaces(query);

        _stateLabel[i] = query;
    }

    string QueryStringParser::getStateLabel(int i){
        return _stateLabel[i];
    }
