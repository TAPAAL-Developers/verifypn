    #include "CodeGenerator.h"
#include "PetriNet.h"
#include <PetriParse/PNMLParser.h>
#include <PetriParse/QueryXMLParser.h>

#include <cstring>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <stdio.h>
#include <string.h>





using namespace std;
namespace PetriEngine{

    CodeGenerator::CodeGenerator(PetriNet* net, MarkVal* m0, PNMLParser::InhibitorArcList inhibarcs, string statelabel, bool isReachBound, bool isPlaceBound, bool quickSolve) {
        _net = net;
        _statelabel = statelabel;
        _nplaces = net->numberOfPlaces();
        _ntransitions = net->numberOfTransitions();
        _m0 = m0;
        _inhibarcs = inhibarcs;
        _isReachBound = isReachBound;
        _isPlaceBound = isPlaceBound;
        _quickSolve = quickSolve;
    }

    int CodeGenerator::inhibArc(unsigned int p, unsigned int t){
        PNMLParser::InhibitorArcIter arcIter;
        for (arcIter = _inhibarcs.begin(); arcIter != _inhibarcs.end(); arcIter++) {
            if (_net->placeNames()[p] == arcIter->source && _net->transitionNames()[t] == arcIter->target) {
                return arcIter->weight;
            }
        }
        return 0;
    }

    char const*CodeGenerator::sl(){
        return _statelabel.c_str();
    }

    void CodeGenerator::generateSource(bool *searchAllPaths, int query_id){
        int group_count, state_count, label_count, dest = 0; 

        int p,t, q, nested_count;
        char sourcename[] = "autogenerated/autogeneratedfile.c";

        group_count = _ntransitions;
        state_count = _nplaces;
        label_count = 1;

        FILE *successor_generator;
        successor_generator = fopen("temp.txt", "w+");

        //Preliminaries
        const char* solvedArray = "solved";
        const char* ComputeBoundsArray = "MaxNumberOfTokensInPlace";
        fputs("#include <ltsmin/pins.h>\nstatic const int LABEL_GOAL = 0;\n", successor_generator);
        fprintf(successor_generator, "int group_count() {return %d;}\nint state_length() {return %d;}\nint label_count() {return %d;}\n", group_count, state_count, label_count);

       if (_quickSolve){
            fprintf(successor_generator, "static int %s[%d] = {", solvedArray, query_id);
            for(q = 0; q<query_id; q++){
                if(q == query_id-1) // dont print a comma after the last query
                    fprintf(successor_generator, "0\n");
                else
                    fprintf(successor_generator, "0,"); // all but the last query
            }
            fprintf(successor_generator, "};\n");
        }


        if (_isReachBound || _isPlaceBound){
        fprintf(successor_generator, "int %s[%d] = {", ComputeBoundsArray, _nplaces);

            for (p = 0; p < _nplaces; p++){
                if(p == _nplaces-1)
                    fprintf(successor_generator, "%d", _m0[p]); 
                else
                    fprintf(successor_generator, "%d,", _m0[p]); 
            }

            fputs("};\n", successor_generator);
        }

        //Write do_callback
        fputs("void do_callback(int *cpy, int *placemarkings, TransitionCB callback, void *arg){\n", successor_generator);
        fputs("int action[1];\ntransition_info_t transition_info = { action, group_count() };\n", successor_generator);
        fprintf(successor_generator, "int dst[%d];\n", _nplaces);
        fprintf(successor_generator, "memcpy(dst, placemarkings, sizeof(int) * %d);\n",  _nplaces);
        fputs("callback(arg, &transition_info, dst, cpy);\n}\n", successor_generator);

        //Write next_state
        fputs("\nint next_state(void* model, int group, int *src, TransitionCB callback, void *arg) {\n", successor_generator);
        fprintf(successor_generator, "(void)model; //ignore\nint cpy[%d];\nint i;\n", _nplaces);
        fputs("for(i=0; i < sizeof(cpy) / sizeof(int); i++ ){ cpy[i] = 1; }\n", successor_generator);
        fputs("int successors = 0;\n", successor_generator);
        fprintf(successor_generator, "int placemarkings[%d];\n", _nplaces);

        // Checking fireability for each transition
        for (t = 0; t < _ntransitions; t++){
            nested_count = 0;

            // Checking fireability conditions in each place
            for (p = 0; p < _nplaces; p++){
                // Condition for regular arcs
                if (_net->inArc(p,t) > 0) {
                    fprintf(successor_generator, "if (src[%d] >= %d) {\n", p, _net->inArc(p,t));
                    nested_count++;
                }
                // Condition for inhibitor arcs
                else if(inhibArc(p,t) > 0){
                    fprintf(successor_generator, "if (src[%d] < %d) {\n", p, inhibArc(p,t));
                    nested_count++;
                }
            }

            fputs("successors++;\n", successor_generator);
            fprintf(successor_generator, "memcpy(placemarkings, src,  sizeof(int) * %d);\n", _nplaces);

            /* The following 2 for-loops should be merged together to gain performace.
                Letting it be as is for the moment to easily check consistency with old codegenerator */

            // Subtract tokens from ingoing places
            for (p = 0; p < _nplaces; p++){
                if (_net->inArc(p,t) > 0) {
                    fprintf(successor_generator, "placemarkings[%d] = src[%d] - %d;\n", p, p, _net->inArc(p,t));
                    fprintf(successor_generator, "cpy[%d] = 0;\n", p);
                }
            }

            // Add tokens to outgoing places
            for(p = 0; p< _nplaces; p++){
                if(_net->outArc(t,p) > 0){
                    fprintf(successor_generator, "placemarkings[%d] = src[%d] + %d;\n", p, p, _net->outArc(t,p));
                    fprintf(successor_generator, "cpy[%d] = 0;\n", p);
                    if(_isReachBound || _isPlaceBound) {fprintf(successor_generator, "if(placemarkings[%d] >= %s[%d]) { %s[%d] = placemarkings[%d]; }\n", p, ComputeBoundsArray, p, ComputeBoundsArray, p, p);}
                }
            }


            fputs("do_callback(cpy, placemarkings, callback, arg);\n", successor_generator);
            fputs("for(i=0; i < sizeof(cpy) / sizeof(int); i++ ){ cpy[i] = 1; }\n", successor_generator);

            for (p = 0; p < nested_count; ++p){
                fputs("}\n", successor_generator);
            }
        }

        fputs("return successors;\n}\n", successor_generator);

        //Write Initial state
        fprintf(successor_generator, "int initial[%d];\n", state_count);
        fputs("int* initial_state() {\n", successor_generator);

        for (p = 0; p < _nplaces; ++p){
            fprintf(successor_generator, "initial[%d] = %d;\n", p, _m0[p]);
        }

        fputs("return initial;\n}\n", successor_generator);



        /* ------------------------------ State Label  ------------------------------ */
        fputs("int state_label(void* model, int label, int* src) {\n", successor_generator);

        if (!_isReachBound){
                fprintf(successor_generator, "if(%s){fprintf(stderr, \"#Query %d is satisfied.\"); return label == LABEL_GOAL && 1;}\n", sl(), query_id);
                fprintf(successor_generator, "return label == LABEL_GOAL && 0;\n}\n");
        }

        // ReachabilityBounds without short circuit
        else if (_isReachBound && !_quickSolve){
             fprintf(successor_generator, "return label == LABEL_GOAL && 0;\n}\n");

        }

        // ReachabilityBounds with short circuit
        else if(_quickSolve){
            fprintf(successor_generator, "if(solved[%d] == 0) %s \n", query_id, sl());
            fprintf(successor_generator, "if (solved[%d] == 1) {return label == LABEL_GOAL && 1;}\n\n", query_id);
            fprintf(successor_generator, "return label == LABEL_GOAL && 0;\n}\n");
        }


        /* ------------------------------ Exit Function ------------------------------ */

        // ReachabilityBounds without short circuit
        if (_isReachBound && !_quickSolve){
            fprintf(successor_generator, "void exit_func(void* model){  \n");

            fprintf(successor_generator, "if(%s){fprintf(stderr, \"#Query %d is satisfied.\");} \n", sl(), query_id);
            fprintf(successor_generator, "fprintf( stderr, \"exiting now\");");
            fprintf(successor_generator, "}"); // end exit_func

        } 

        // ReachabilityBounds with short circuit
        else if (_quickSolve) {
            fprintf(successor_generator, "void exit_func(void* model){  \n");
            fprintf(successor_generator, "if(solved[%d] == 0){fprintf(stderr, \"#Query %d is satisfied.\");} \n", query_id, query_id);
            fprintf(successor_generator, "fprintf( stderr, \"exiting now\");");
            fputs("}", successor_generator);  // end exit_func
        }

        // PlaceBounds
        else if (_isPlaceBound){ 
            int p = 0;
            fprintf(successor_generator, "void exit_func(void* model){  \n");
            size_t startPos = 0;
            string query = sl();
            fprintf(successor_generator, " fprintf(stderr, \"Query %d max tokens are \'", query_id);
            fputs("%d\'.\\n\",", successor_generator);

            while((startPos = query.find("[", startPos)) != std::string::npos) {
                size_t end_quote = query.find("]", startPos + 1);
                size_t nameLen = (end_quote - startPos) + 1;
                string oldPlaceName = query.substr(startPos + 1, nameLen - 2);   
                startPos++;
                
                if(p != 0)
                    fprintf(successor_generator, "+");

                fprintf(successor_generator, "MaxNumberOfTokensInPlace[%s]", oldPlaceName.c_str());
                p++;
            }
            fprintf(successor_generator, ");\n");
            fprintf(successor_generator, "fprintf( stderr, \"exiting now\");");
            fputs("}", successor_generator);  // end exit_func
        }

        // Other Boolean Formula
        else {
            fprintf(successor_generator, "void exit_func(void* model){ fprintf(stderr, \"exiting now\");} \n");
        }


        fclose(successor_generator);
        int result = rename("temp.txt", sourcename);
    }

        // Verifying all queries in one go
        void CodeGenerator::generateSourceMultipleQueries(std::vector<std::string> *statelabels, int *solved, bool *searchAllPaths, int numberOfQueries){
        int group_count, state_count, label_count, dest = 0;
        int p,t,q, nested_count;
        char sourcename[] = "autogenerated/autogeneratedfile.c";

        group_count = _ntransitions;
        state_count = _nplaces;
        label_count = 1;

        FILE *successor_generator;
        successor_generator = fopen("temp.txt", "w+");

        const char* solvedArray = "solved";
        const char* ComputeBoundsArray = "MaxNumberOfTokensInPlace";



        //Preliminaries
        fputs("#include <ltsmin/pins.h>\nstatic const int LABEL_GOAL = 0;\n", successor_generator);


        // declare and assign solved queries
        fprintf(successor_generator, "static int %s[%d] = {", solvedArray, numberOfQueries);
        for(q = 0; q<numberOfQueries; q++){
            if(q == numberOfQueries-1) // dont print a comma after the last query
                fprintf(successor_generator, "0};\n");
            else
                fprintf(successor_generator, "0,"); // all but the last query
        }

        if (_isReachBound || _isPlaceBound){
        fprintf(successor_generator, "static int %s[%d] = {", ComputeBoundsArray, _nplaces);
            for(q = 0; q<_nplaces; q++){
                if(q == _nplaces-1) // dont print a comma after the last query
                    fprintf(successor_generator, "%d};\n", _m0[q]);
                else
                    fprintf(successor_generator, "%d,", _m0[q]); // all but the last query
            }
        }



        fprintf(successor_generator, "int group_count() {return %d;}\nint state_length() {return %d;}\nint label_count() {return %d;}\n", group_count, state_count, label_count);

        //Write do_callback
        fputs("void do_callback(int *cpy, int *placemarkings, TransitionCB callback, void *arg){\n", successor_generator);
        fputs("int action[1];\ntransition_info_t transition_info = { action, group_count() };\n", successor_generator);
        fprintf(successor_generator, "int dst[%d];\n", _nplaces);
        fprintf(successor_generator, "memcpy(dst, placemarkings, sizeof(int) * %d);\n",  _nplaces);
        fputs("callback(arg, &transition_info, dst, cpy);\n}\n", successor_generator);

        //Write next_state
        fputs("\nint next_state(void* model, int group, int *src, TransitionCB callback, void *arg) {\n", successor_generator);
        fprintf(successor_generator, "(void)model; //ignore\nint cpy[%d];\nint i;\n", _nplaces);
        fputs("for(i=0; i < sizeof(cpy) / sizeof(int); i++ ){ cpy[i] = 1; }\n", successor_generator);
        fputs("int successors = 0;\n", successor_generator);
        fprintf(successor_generator, "int placemarkings[%d];\n", _nplaces);

        // Checking fireability for each transition
        for (t = 0; t < _ntransitions; t++){
            nested_count = 0;

            // Checking fireability conditions in each place
            for (p = 0; p < _nplaces; p++){
                // Condition for regular arcs
                if (_net->inArc(p,t) > 0) {
                    fprintf(successor_generator, "if (src[%d] >= %d) {\n", p, _net->inArc(p,t));
                    nested_count++;
                }
                // Condition for inhibitor arcs
                else if(inhibArc(p,t) > 0){
                    fprintf(successor_generator, "if (src[%d] < %d) {\n", p, inhibArc(p,t));
                    nested_count++;
                }
            }

            fputs("successors++;\n", successor_generator);
            fprintf(successor_generator, "memcpy(placemarkings, src,  sizeof(int) * %d);\n", _nplaces);

            /* The following 2 for-loops should be merged together to gain performace.
                Letting it be as is for the moment to easily check consistency with old codegenerator */

            // Subtract tokens from ingoing places
            for (p = 0; p < _nplaces; p++){
                if (_net->inArc(p,t) > 0) {
                    fprintf(successor_generator, "placemarkings[%d] = src[%d] - %d;\n", p, p, _net->inArc(p,t));
                    fprintf(successor_generator, "cpy[%d] = 0;\n", p);
                }
            }

            // Add tokens to outgoing places
            for(p = 0; p< _nplaces; p++){
                if(_net->outArc(t,p) > 0){
                    fprintf(successor_generator, "placemarkings[%d] = src[%d] + %d;\n", p, p, _net->outArc(t,p));
                    fprintf(successor_generator, "cpy[%d] = 0;\n", p);
                    if(_isReachBound || _isPlaceBound) {fprintf(successor_generator, "if(placemarkings[%d] >= %s[%d]) { %s[%d] = placemarkings[%d]; }\n", p, ComputeBoundsArray, p, ComputeBoundsArray, p, p);}
                }
            }

            fputs("do_callback(cpy, placemarkings, callback, arg);\n", successor_generator);
            fputs("for(i=0; i < sizeof(cpy) / sizeof(int); i++ ){ cpy[i] = 1; }\n", successor_generator);

            for (p = 0; p < nested_count; ++p){
                fputs("}\n", successor_generator);
            }
        }

        fputs("return successors;\n}\n", successor_generator);

        //Write Initial state
        fprintf(successor_generator, "int initial[%d];\n", state_count);
        fputs("int* initial_state() {\n", successor_generator);

        for (p = 0; p < _nplaces; ++p){
            fprintf(successor_generator, "initial[%d] = %d;\n", p, _m0[p]);
        }

        fputs("return initial;\n}\n", successor_generator);

        //Write State_label
        fputs("int state_label(void* model, int label, int* src) {\n", successor_generator);

        if (!_isReachBound ){
            for(q = 0; q<numberOfQueries; q++){
                if (solved[q] != 1)
                    fprintf(successor_generator, "if(%s[%d] == 0){if(%s){%s[%d] = 1;fprintf(stderr, \"#Query %d is satisfied.\");}}\n", solvedArray, q, statelabels->at(q).c_str(), solvedArray, q, q);
            }
        } else if (_quickSolve){
            for(q = 0; q<numberOfQueries; q++){
                    
                    fprintf(successor_generator, "if(%s[%d] == 0) %s \n", solvedArray, q, statelabels->at(q).c_str());
            }
        }
        else {
            for(q = 0; q<numberOfQueries; q++){
                    fprintf(successor_generator, "if(%s[%d] == 0){if(0){%s[%d] = 1;}}\n", solvedArray, q, solvedArray, q);
            }

        }


        // return true if all queries are verified
        /*
        fprintf(successor_generator, "if(");
        for(q = 0; q<numberOfQueries; q++){
            if(q == numberOfQueries-1)
                fprintf(successor_generator, "%s[%d]){ fprintf(stdout, \"\\n# ALL QUERIES HAS BEEN VERIFIED.\\n\"; return label == LABEL_GOAL && 1;}\n", solvedArray, q);
            else
                fprintf(successor_generator, "%s[%d]&&", solvedArray, q);
        }
        */


        fprintf(successor_generator, "return label == LABEL_GOAL && 0;\n}\n");


        if (_isReachBound && !_quickSolve){
        fprintf(successor_generator, "void exit_func(void* model){  \n");

        for(q = 0; q<numberOfQueries; q++){

                fprintf(successor_generator, "if(%s[%d] == 0){if(%s){%s[%d] = 1;fprintf(stdout, \"#Query %d is satisfied.\\n\");}}\n", solvedArray, q, statelabels->at(q).c_str(), solvedArray, q, q);
            }

        fprintf(successor_generator, "fprintf( stderr, \"exiting now\");");
            fputs("}",successor_generator);
            
        }
        else if(_quickSolve){
             fprintf(successor_generator, "void exit_func(void* model){  \n");

            for(q = 0; q<numberOfQueries; q++){
                               fprintf(successor_generator, "if(solved[%d] == 0){fprintf(stderr, \"#Query %d is satisfied.\");} \n", q, q);
                            }
            fprintf(successor_generator, "fprintf( stderr, \"exiting now\");}");

        }
            
        else if (_isPlaceBound){
            int p;
            fprintf(successor_generator, "void exit_func(void* model){ \n");

            for(q = 0; q<numberOfQueries; q++){
                size_t startPos = 0;
                p = 0;
// -- new
                fprintf(successor_generator, " fprintf(stderr, \"Query %d max tokens are \'", q);
                fputs("%d\'.\\n\",", successor_generator);

                while((startPos = statelabels->at(q).find("[", startPos)) != std::string::npos) {
                    size_t end_quote = statelabels->at(q).find("]", startPos + 1);
                    size_t nameLen = (end_quote - startPos) + 1;
                    string oldPlaceName = statelabels->at(q).substr(startPos + 1, nameLen - 2);   
                    startPos++;
                    
                    if(p != 0)
                        fprintf(successor_generator, "+");

                    fprintf(successor_generator, "MaxNumberOfTokensInPlace[%s]", oldPlaceName.c_str());
                    p++;
                }
                fprintf(successor_generator, ");\n");
// --->

            }   


            fprintf(successor_generator, "fprintf( stderr, \"exiting now\");");       
            fputs("}", successor_generator); 

        }


        else {fprintf(successor_generator, "void exit_func(void* model){ fprintf(stderr, \"exiting now\");} \n");}

        fclose(successor_generator);
        int result = rename("temp.txt", sourcename);
    
    }   


    void CodeGenerator::generateSourceForSSE(){
        int group_count, state_count, label_count, dest = 0; 

        int p,t, nested_count;
        char sourcename[] = "autogenerated/autogeneratedfile.c";

        group_count = _ntransitions;
        state_count = _nplaces;
        label_count = 1;

        FILE *successor_generator;
        successor_generator = fopen("temp.txt", "w+");

        const char* ComputeBoundsArray = "MaxNumberOfTokensInPlace";

        //Preliminaries

        fputs("#include <ltsmin/pins.h>\nstatic const int LABEL_GOAL = 0;\n", successor_generator);
        fprintf(successor_generator, "int group_count() {return %d;}\nint state_length() {return %d;}\nint label_count() {return %d;}\n", group_count, state_count, label_count);

        fprintf(successor_generator, "int %s[%d] = {\n", ComputeBoundsArray, _nplaces);
        for (p = 0; p < _nplaces; p++){
        fprintf(successor_generator, "%d, \n", _m0[p]);
        }

        fputs("};", successor_generator);
        

        //Write do_callback
        fputs("void do_callback(int *cpy, int *placemarkings, TransitionCB callback, void *arg){\n", successor_generator);
        fputs("int action[1];\ntransition_info_t transition_info = { action, group_count() };\n", successor_generator);
        fprintf(successor_generator, "int dst[%d];\n", _nplaces);
        fprintf(successor_generator, "memcpy(dst, placemarkings, sizeof(int) * %d);\n",  _nplaces);
        fputs("callback(arg, &transition_info, dst, cpy);\n}\n", successor_generator);

        //Write next_state
        fputs("\nint next_state(void* model, int group, int *src, TransitionCB callback, void *arg) {\n", successor_generator);
        fprintf(successor_generator, "(void)model; //ignore\nint cpy[%d];\nint i;\n", _nplaces);
        fputs("for(i=0; i < sizeof(cpy) / sizeof(int); i++ ){ cpy[i] = 1; }\n", successor_generator);
        fputs("int successors = 0;\n", successor_generator);
        fprintf(successor_generator, "int placemarkings[%d];\n", _nplaces);

        // Checking fireability for each transition
        for (t = 0; t < _ntransitions; t++){
            nested_count = 0;

            // Checking fireability conditions in each place
            for (p = 0; p < _nplaces; p++){
                // Condition for regular arcs
                if (_net->inArc(p,t) > 0) {
                    fprintf(successor_generator, "if (src[%d] >= %d) {\n", p, _net->inArc(p,t));
                    nested_count++;
                }
                // Condition for inhibitor arcs
                else if(inhibArc(p,t) > 0){
                    fprintf(successor_generator, "if (src[%d] < %d) {\n", p, inhibArc(p,t));
                    nested_count++;
                }
            }

            fputs("successors++;\n", successor_generator);
            fprintf(successor_generator, "memcpy(placemarkings, src,  sizeof(int) * %d);\n", _nplaces);

            /* The following 2 for-loops should be merged together to gain performace.
                Letting it be as is for the moment to easily check consistency with old codegenerator */

            // Subtract tokens from ingoing places
            for (p = 0; p < _nplaces; p++){
                if (_net->inArc(p,t) > 0) {
                    fprintf(successor_generator, "placemarkings[%d] = src[%d] - %d;\n", p, p, _net->inArc(p,t));
                    fprintf(successor_generator, "cpy[%d] = 0;\n", p);
                }
            }

            // Add tokens to outgoing places
            for(p = 0; p< _nplaces; p++){
                if(_net->outArc(t,p) > 0){
                    fprintf(successor_generator, "placemarkings[%d] = src[%d] + %d;\n", p, p, _net->outArc(t,p));
                    fprintf(successor_generator, "cpy[%d] = 0;\n", p);
                    fprintf(successor_generator, "if(placemarkings[%d] >= %s[%d]) { %s[%d] = placemarkings[%d]; }\n", p, ComputeBoundsArray, p, ComputeBoundsArray, p, p);
                }
            }


            fputs("do_callback(cpy, placemarkings, callback, arg);\n", successor_generator);
            fputs("for(i=0; i < sizeof(cpy) / sizeof(int); i++ ){ cpy[i] = 1; }\n", successor_generator);

            for (p = 0; p < nested_count; ++p){
                fputs("}\n", successor_generator);
            }
        }

        fputs("return successors;\n}\n", successor_generator);

        //Write Initial state
        fprintf(successor_generator, "int initial[%d];\n", state_count);
        fputs("int* initial_state() {\n", successor_generator);

        for (p = 0; p < _nplaces; ++p){
            fprintf(successor_generator, "initial[%d] = %d;\n", p, _m0[p]);
        }

        fputs("return initial;\n}\n", successor_generator);

        //Write State_label
        fputs("int state_label(void* model, int label, int* src) {\n", successor_generator);
        fprintf(successor_generator, "return label == LABEL_GOAL && 0;\n}\n");



        fprintf(successor_generator, "void exit_func(void* model){  \n");

        fputs("int T = 0; int M = 0;\n", successor_generator);
        for (int i = 0; i < _nplaces; i++){
            fprintf(successor_generator, "T += MaxNumberOfTokensInPlace[%d];\n if(M < MaxNumberOfTokensInPlace[%d]) { M = MaxNumberOfTokensInPlace[%d]; }", i, i, i);
        }
        fputs("fprintf(stderr, \"Maximum number of tokens in marking \'%d\' \\n\", T);\n", successor_generator);
        fputs("fprintf(stderr, \"Maximum number of tokens in one Place \'%d\' \", M);\n", successor_generator);

        fprintf(successor_generator, "fprintf( stderr, \"exiting now\");}");
        

        fclose(successor_generator);
        int result = rename("temp.txt", sourcename);
    }













    // Generates dummy values until queries can be propery parsed.
    // Final version should include parameter of the XMLparser queries vector to get the proper queryText.
    void CodeGenerator::createQueries(string *stringQueries, int *negateResult, QueryXMLParser::Queries queries, vector<string> stateLabels){
            int q;
            for(q = 0; q < queries.size(); q++){
                if(queries[q].negateResult){
                    negateResult[q] = 1;
                }
                else{
                    negateResult[q] = 0;
                }
                stringQueries[q] = stateLabels[q];
            }
            /*
            stringQueries[0] = "src[2]>0";
            stringQueries[1] = "src[8]>3";
            stringQueries[2] = "src[3]>2&&src[4] > 2";
            stringQueries[3] = "src[0]>0&&src[2]>0&&src[5]>0";
            stringQueries[4] = "src[0]>0&&src[2]>0&&src[4]>0";
            */
    }

    void CodeGenerator::printQueries(std::vector<std::string>  queries, int numberOfQueries){
        int q;
        fprintf(stdout, "** Queries in code generator **\n");
        for(q = 0; q<numberOfQueries; q++){
            fprintf(stdout, "Query %d: %s\n", q, queries[q].c_str());
        }
    }

} //PetriEngine
