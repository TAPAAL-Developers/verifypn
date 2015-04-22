#include "QueryStringParser.h"

#include <stdio.h>
#include <string>
#include <string.h>
#include <iostream>
#include <sstream>

using namespace std;

    QueryStringParser::QueryStringParser(QueryXMLParser *Parser, PetriEngine::PetriNet *PetriNet, PNMLParser::InhibitorArcList inhibarcs){
        _Parser = Parser;
        _PetriNet = PetriNet;
        _inhibArcs = inhibarcs;

        // Create placeholders in new statelabel vector
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
            startPos += newPlaceName.size();
        }
    }

    void QueryStringParser::convertToComputeBoundsQuery(std::string& query) {
        size_t startPos = 0;

        while((startPos = query.find("\"", startPos)) != std::string::npos) {
            size_t end_quote = query.find("\"", startPos + 1);
            size_t nameLen = (end_quote - startPos) + 1;

            // Exclude both quotes from place name before searching
            string oldPlaceName = query.substr(startPos + 1, nameLen - 2);
            string newPlaceIndex = getPlaceIndexByName(oldPlaceName);
            string newPlaceName = "MaxNumberOfTokensInPlace[" + newPlaceIndex + "]";

            query.replace(startPos, nameLen, newPlaceName);
            startPos += newPlaceName.size();
        }
    }

     bool QueryStringParser::convertToBoundsQuery(std::string& query){
        size_t startPos = 0;

        while((startPos = query.find("<=", startPos)) != std::string::npos) {

            size_t second_parameter = startPos + 3;
            size_t first_parameter = startPos - 2;

            if (query.substr(first_parameter, 1) == "]" && query.substr(second_parameter, 1) == "M") return false;
            startPos += second_parameter;
        }

        startPos = 0;

        while((startPos = query.find(">=", startPos)) != std::string::npos) {
            size_t second_parameter = startPos + 4;
            size_t first_parameter = startPos - 2;

            if (query.substr(first_parameter, 1) == "]" && query.substr(second_parameter, 1) == "M") return false;
            startPos += second_parameter;
        }

        return true;
        

    }

    void QueryStringParser::completeShortCut(std::string& query, int i){
        size_t startPos = 0;
        bool isModified = false;
        while((startPos = query.find("<=", startPos)) != std::string::npos) {
            isModified = true;
            string newSign = ">";
            query.replace(startPos, 2, newSign);
            startPos += 2;
        }

        stringstream ss;
        ss << i;
        string is = ss.str();

        if (isModified){
        string temp = query;
        query = string("{if(") + temp.c_str() + ") {solved[" + is.c_str() + "] = 1; fprintf(stderr, \"#Query " + is.c_str() + " is NOT satisfied.\");}}\n";}
        else { string temp = query;
        query = string("{if(") + temp.c_str() + ") {solved[" + is.c_str() + "] = 1; fprintf(stderr, \"#Query " + is.c_str() + " is satisfied.\");}} \n";}


    }

    int QueryStringParser::inhibArc(unsigned int p, unsigned int t){
        for (PNMLParser::InhibitorArcIter it = _inhibArcs.begin(); it != _inhibArcs.end(); it++) {
            if (_PetriNet->placeNames()[p] == it->source && _PetriNet->transitionNames()[t] == it->target) {
                return it->weight;
            }
        }
        return 0;
    }

    void QueryStringParser::findDeadlockConditions(std::string& query, size_t deadlockPos){
        int t, p;
        bool beginningConjunction = true;
        size_t startPos = deadlockPos;
        unsigned int nPlaces = _PetriNet->numberOfPlaces();
        unsigned int nTransitions = _PetriNet->numberOfTransitions();
        string conditions;
        std::ostringstream s;

        // Negate results
        conditions += "!(";

        // Checking fireability for each transition
        for(t = 0; t < nTransitions; t++){
            // Checking fireability conditions in each place
            for(p = 0; p < nPlaces; p++){
                // Condition for regular arcs
                if(_PetriNet->inArc(p,t) > 0) {
                    if(t > 0)
                        conditions += " && ";

                    s << "(src[" << p << "] >= " << _PetriNet->inArc(p,t) << ")";
                    
                    conditions += s.str();

                    if(beginningConjunction){
                        conditions += " && ";
                        beginningConjunction = false;
                    }
                }
                // Condition for inhibitor arcs
                else if(inhibArc(p,t) > 0){
                    if(t > 0)
                        conditions += " && ";

                    s << "(src[" << p << "] < " << _PetriNet->inArc(p,t) << ")";

                    conditions += s.str();

                    if(beginningConjunction){
                        conditions += " && ";
                        beginningConjunction = false;
                    }
                }


                s.str("");
            }
        }

        conditions += " )";
        query.replace(deadlockPos, 8, conditions);
    }

    void QueryStringParser::generateStateLabel(int i){
        QueryXMLParser::QueryItem queryItem = _Parser->queries[i];
        string query = queryItem.queryText;

        //cout <<"\n\n&&& Query: "<<query<<endl;

        // Remove temporal operators EF
        if (!_Parser->queries[i].isPlaceBound)
        query.replace(0, 2, "");

        // Check if query is of type ReachabilityDeadlock
        size_t deadlockPos = query.find("deadlock", 0);

        if(deadlockPos != std::string::npos){
            findDeadlockConditions(query, deadlockPos);
        } else if(queryItem.isPlaceBound){ // ReacabilityComputeBounds query
            convertToComputeBoundsQuery(query);
        } else if(queryItem.isReachBound){ // ReachabilityBounds query
            convertToComputeBoundsQuery(query);
            if(convertToBoundsQuery(query)){
            _Parser->queries[i].quickSolve = true;
            completeShortCut(query, i);
            }
        } else { //
            // Rename place names eg. "place0" -> src[0]
            replacePlaces(query);
        }

        // Replace all TAPAAL query operators with C operators
        replaceOperator(query, "not", "!");
        replaceOperator(query, "and", "&&");
        replaceOperator(query, "or", "||");

        // Replace true/false with 1,0
        replaceOperator(query, "true", "1");
        replaceOperator(query, "false", "0");

        cout<<"VALUE OF QUICKSOLVE: "<< _Parser->queries[i].quickSolve <<" ms\n"<<endl;

        _stateLabel[i] = query;
    }

    void QueryStringParser::generateStateLabels(){
        int i;
        for(i = 0; i < _Parser->queries.size(); i++){
            QueryStringParser::generateStateLabel(i);
        }
    }

    string QueryStringParser::getStateLabel(int i){
        return _stateLabel[i];
    }

    std::vector<std::string> QueryStringParser::getStateLabels(){
        return _stateLabel;
    }
