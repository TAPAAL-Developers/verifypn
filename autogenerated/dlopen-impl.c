#include <ltsmin/pins.h>
#include <ltsmin/dlopen-api.h>
#include <ltsmin/ltsmin-standard.h>
#include <ltsmin/lts-type.h>

#include <autogen.h>

// set the name of this PINS plugin
char pins_plugin_name[] = "TAPAAL Distributed Engine";

void pins_model_init(model_t m) {

    // create the LTS type LTSmin will generate
    lts_type_t ltstype=lts_type_create();

    // set the length of the state
    lts_type_set_state_length(ltstype, state_length());

    // add an "int" type for a state slot
    int int_type = lts_type_add_type(ltstype, "int", NULL);
    lts_type_set_format (ltstype, int_type, LTStypeDirect);

    // add an "action" type for edge labels
    /*int action_type = lts_type_add_type(ltstype, "action", NULL);
    lts_type_set_format (ltstype, action_type, LTStypeEnum);*/

    // add a "bool" type for state labels
    int bool_type = lts_type_add_type (ltstype, LTSMIN_TYPE_BOOL, NULL);
    lts_type_set_format(ltstype, bool_type, LTStypeEnum);

    // set state name & type
    for (int i=0; i < state_length(); ++i) {
        char name[3]; sprintf(name, "%d", i);
        lts_type_set_state_name(ltstype,i,name);
        lts_type_set_state_typeno(ltstype,i,int_type);
    }

    // edge label types
    //lts_type_set_edge_label_count (ltstype, 1);
    /*lts_type_set_edge_label_name(ltstype, 0, "action");
    lts_type_set_edge_label_type(ltstype, 0, "action");
    lts_type_set_edge_label_typeno(ltstype, 0, action_type);*/

    // state label types
    lts_type_set_state_label_count (ltstype, 1);
    lts_type_set_state_label_name (ltstype, 0, "goal");
    lts_type_set_state_label_typeno (ltstype, 0, bool_type);

    // done with ltstype
    lts_type_validate(ltstype);

    // make sure to set the lts-type before anything else in the GB
    GBsetLTStype(m, ltstype);

    // setting all values for all non direct types
   /* GBchunkPut(m, action_type, chunk_str("walk_left"));
    GBchunkPut(m, action_type, chunk_str("push_left"));
    GBchunkPut(m, action_type, chunk_str("walk_right"));*/
    GBchunkPut(m, bool_type, chunk_str(LTSMIN_VALUE_BOOL_FALSE));
    GBchunkPut(m, bool_type, chunk_str(LTSMIN_VALUE_BOOL_TRUE));

    // set state variable values for initial state
    GBsetInitialState(m, initial_state());

    // set function pointer for the next-state function
    GBsetNextStateLong(m, (next_method_grey_t) next_state);

    // set function pointer for the label evaluation function
    GBsetStateLabelLong(m, (get_label_method_t) state_label);


    int exit_func(void* model);
    GBsetExit(m, exit_func);

    // create combined matrix
    matrix_t *cm = malloc(sizeof(matrix_t));
    dm_create(cm, group_count(), state_length());

    // set the read dependency matrix
    matrix_t *rm = malloc(sizeof(matrix_t));
    dm_create(rm, group_count(), state_length());
    for (int i = 0; i < group_count(); i++) {
        for (int j = 0; j < state_length(); j++) {

                dm_set(cm, i, j);
                dm_set(rm, i, j);

        }
    }
    GBsetDMInfoRead(m, rm);

    // set the write dependency matrix
    matrix_t *wm = malloc(sizeof(matrix_t));
    dm_create(wm, group_count(), state_length());
    for (int i = 0; i < group_count(); i++) {
        for (int j = 0; j < state_length(); j++) {

                dm_set(cm, i, j);
                dm_set(wm, i, j);

        }
    }
    GBsetDMInfoMustWrite(m, wm);

    // set the combined matrix
    GBsetDMInfo(m, cm);

    // set the label dependency matrix
    matrix_t *lm = malloc(sizeof(matrix_t));
    dm_create(lm, label_count(), state_length());
    for (int i = 0; i < label_count(); i++) {
        for (int j = 0; j < state_length(); j++) {
            dm_set(lm, i, j);
        }
    }
    GBsetStateLabelInfo(m, lm);

}
