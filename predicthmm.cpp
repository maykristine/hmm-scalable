/*
 
 Copyright (c) 2012-2017, Michael (Mikhail) Yudelson
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 * Neither the name of the Michael (Mikhail) Yudelson nor the
 names of other contributors may be used to endorse or promote products
 derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 */

#include <string.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <map>
#include <list>
#include "utils.h"
#include "HMMProblem.h"
#include "InputUtil.h"
//#include "HMMProblemPiG.h"
#include "HMMProblemPiGK.h"
#include "HMMProblemAGK.h"
#include "HMMProblemPiAGK.h"
#include "HMMProblemPiABGK.h"
#include "HMMProblemComp.h"
#include "HMMProblemSlicedAB.h"
#include "HMMProblemSlicedA.h"
#include "HMMProblemPiGKww.h"

using namespace std;

#define COLUMNS 4

struct param param;
//static char *line = NULL;
NUMBER* metrics;
map<string,NCAT> data_map_group_fwd;
map<NCAT,string> data_map_group_bwd;
map<string,NCAT> map_step;
map<string,NCAT> model_map_skill_fwd;
map<NCAT,string> model_map_skill_bwd;
void exit_with_help();
void parse_arguments(int argc, char **argv, char *input_file_name, char *model_file_name, char *predict_file_name);
void read_predict_data(const char *filename);
void predict(const char *predict_file, HMMProblem *hmm);

int main (int argc, char ** argv) {
	clock_t tm0 = clock();
	printf("predicthmm starting...\n");
	set_param_defaults(&param);
	
	char input_file[1024];
	char model_file[1024];
	char predict_file[1024];
	
	parse_arguments(argc, argv, input_file, model_file, predict_file);
    // param.predictions = 2; // do not force it on

    // read data
    if(param.binaryinput==0) {
        InputUtil::readTxt(input_file, &param);
    } else {
        InputUtil::readBin(input_file, &param);
    }
    
    // read model header
	FILE *fid = fopen(model_file,"r");
	if(fid == NULL)
	{
		fprintf(stderr,"Can't read model file %s\n",model_file);
		exit(1);
	}
	int max_line_length = 1024;
	char *line = Malloc(char,(size_t)max_line_length);
	NDAT line_no = 0;
    struct param param_model;
    set_param_defaults(&param_model);
    bool overwrite = false;
//    if(overwrite)
        readSolverInfo(fid, &param_model, &line_no);
//    else
//        readSolverInfo(fid, &initparam, &line_no);
    
    // copy partial info from param_model to param
    if(param.nO==0) param.nO = param_model.nO;
    param.structure = param_model.structure;
    param.solver = param_model.solver;
    param.solver_setting = param_model.solver_setting;
    
    // copy number of states from the model
    param.nS = param_model.nS;
    
    // if number of states or observations >2, then no check
    if( param.nO>2 || param.nS>2)
        param.do_not_check_constraints = 1;
    
	// copy number of slices from the model
	param.nZ = param_model.nZ;
    //
    // create hmm Object
    //
    HMMProblem *hmm = NULL;
	switch(param.structure)
	{
		case STRUCTURE_SKILL: // Conjugate Gradient Descent
//            case STRUCTURE_GROUP: // Conjugate Gradient Descent
			hmm = new HMMProblem(&param);
			break;
//            case STRUCTURE_PIg: // Gradient Descent: PI by group, A,B by skill
//                hmm = new HMMProblemPiG(&param);
//                break;
		case STRUCTURE_SKABslc: // Conjugate Gradient Descent
			hmm = new HMMProblemSlicedAB(&param);
			break;
		case STRUCTURE_SKAslc: // Conjugate Gradient Descent
			hmm = new HMMProblemSlicedA(&param);
			break;
		case STRUCTURE_PIgk: // Gradient Descent, pLo=f(K,G), other by K
			hmm = new HMMProblemPiGK(&param);
			break;
		case STRUCTURE_PIgkww: // Gradient Descent, pLo=f(K,G), other by K
			hmm = new HMMProblemPiGKww(&param);
			break;
		case STRUCTURE_PIAgk: // Gradient Descent, pLo=f(K,G), pT=f(K,G), other by K
			hmm = new HMMProblemPiAGK(&param);
			break;
		case STRUCTURE_Agk: // Gradient Descent, pT=f(K,G), other by K
			hmm = new HMMProblemAGK(&param);
			break;
//                case STRUCTURE_Agki: // Gradient Descent, pT=f(K,G), other by K
//                    hmm = new HMMProblemAGKi(&param);
//                    break;
		case STRUCTURE_PIABgk: // Gradient Descent, pT=f(K,G), other by K
			hmm = new HMMProblemPiABGK(&param);
			break;
//            case BKT_GD_T: // Gradient Descent with Transfer
//                hmm = new HMMProblemKT(&param);
//                break;
		case STRUCTURE_COMP: // Gradient Descent, pT=f(K,G), other by K
			hmm = new HMMProblemComp(&param);
			break;
	}
    // read model body
    hmm->readModelBody(fid, &param_model, &line_no, overwrite);
  	fclose(fid);
	free(line);
	
	if(param.quiet == 0)
        printf("input read, nO=%d, nG=%d, nK=%d, nI=%d, nZ=%d\n",param.nO, param.nG, param.nK, param.nI, param.nZ);
	
	clock_t tm = clock();
//    if(param.metrics>0 || param.predictions>0) {
        metrics = Calloc(NUMBER, (size_t)7);// LL, AIC, BIC, RMSE, RMSEnonull, Acc, Acc_nonull;
//    }
	HMMProblem::predict(metrics, predict_file, param.dat_obs, param.dat_group, param.dat_skill, param.dat_skill_stacked, param.dat_skill_rcount, param.dat_skill_rix, &hmm, 1, NULL);
//    predict(predict_file, hmm);
	if(param.quiet == 0)
		printf("predicting is done in %8.6f seconds\n",(NUMBER)(clock()-tm)/CLOCKS_PER_SEC);
    //if( param.predictions>0 ) {
        printf("trained model LL=%15.7f (%15.7f), AIC=%8.6f, BIC=%8.6f, RMSE=%8.6f (%8.6f), Acc=%8.6f (%8.6f)\n",
               metrics[0], metrics[1], // ll's
               2*hmm->getNparams() + 2*metrics[0], hmm->getNparams()*safelog(param.N) + 2*metrics[0],
               metrics[2], metrics[3], // rmse's
               metrics[4], metrics[5]); // acc's
    //}
    free(metrics);
    
	destroy_input_data(&param);
	
    delete hmm;
	if(param.quiet == 0)
		printf("overall time running is %8.6f seconds\n",(NUMBER)(clock()-tm0)/CLOCKS_PER_SEC);
    return 0;
}

void exit_with_help() {
	printf(
		   "Usage: predicthmm [options] input_file model_file [predicted_response_file]\n"
           "options:\n"
           "-q : quiet mode, without output, 0-no (default), or 1-yes\n"
           "-d : delimiter for multiple skills per observation; 0-single skill per\n"
           "     observation (default), otherwise -- delimiter character, e.g. '-d ~'.\n"
           "-b : treat input file as binary input file (specifications TBA).\n"
           "-p : report model predictions on the train set 0-no (default), 1-yes; 2-yes,\n"
           "     plus output state probability; works with -v and -m parameters.\n"
           "-U : controls how update to the probability distribution of the states is\n"
           "     updated. Takes the following format '-U r|g[,t|g]', where first\n"
           "     character controls how prediction treats known observations, second -- how\n"
           "     prediction treats unknown observations, and third -- whether to output\n"
           "     probabilities of priors. Dealing with known observations 'r' - reveal\n"
           "     actual observations for the update of state probability distribution (makes\n"
           "     sense for modeling how an actual system would work), 'g' - 'guessing' the\n"
           "     observation based on the predicted outcomes (arg max) -- more appropriate\n"
           "     when comparing models (so that no information about observation is never\n"
           "     revealed). Dealing with unknown observations (marked as '.' -- dot): 't' --\n"
           "     use transition matrix only, 'g' -- 'guess' the observation.\n"
           "     Default (if ommitted) is '-U r,t'.\n"
           "     For examle, '-U g,g would require 'guessing' of what the observation was\n"
           "     using model parameters and the running value of the probabilities of state\n"
           "     distributions.\n"
		   );
	exit(1);
}

void parse_arguments(int argc, char **argv, char *input_file_name, char *model_file_name, char *predict_file_name) {
	// parse command line options, starting from 1 (0 is path to executable)
	// go in pairs, looking at whether first in pair starts with '-', if not, stop parsing arguments
	int i, n;
    char * ch;
	for(i=1;i<argc;i++)
	{
		if(argv[i][0] != '-') break; // end of options stop parsing
		if(++i>=argc)
			exit_with_help();
		switch(argv[i-1][1])
		{
			case 'q':
				param.quiet = (NPAR)atoi(argv[i]);
				if(param.quiet!=0 && param.quiet!=1) {
					fprintf(stderr,"ERROR! Quiet param should be 0 or 1\n");
					exit_with_help();
				}
				break;
            case  'd':
				param.multiskill = argv[i][0]; // just grab first character (later, maybe several)
                break;
			case 'b':
                param.binaryinput = atoi( strtok(argv[i],"\t\n\r"));
                break;
            case  'p':
				param.predictions = atoi(argv[i]);
				if(param.predictions<0 || param.predictions>2) {
					fprintf(stderr,"a flag of whether to report predictions for training data (-p) should be 0, 1 or 2\n");
					exit_with_help();
				}
                break;
			case 't':
				param.sliced = (NPAR)atoi(argv[i]);
				if(param.sliced!=0 && param.sliced!=1) {
					fprintf(stderr,"ERROR! Time parameter should be either 0 (off) or 1(on)\n");
					exit_with_help();
				}
				break;
            case  'U':
                param.update_known = *strtok(argv[i],",\t\n\r");
                ch = strtok(NULL, ",\t\n\r");
                param.update_unknown = ch[0];
                
                if( (param.update_known!='r' && param.update_known!='g') ||
                   (param.update_unknown!='t' && param.update_unknown!='g') ) {
                    fprintf(stderr,"specification of how probabilities of states should be updated (-U) is incorrect, it sould be r|g[,t|g] \n");
                    exit_with_help();
                }
                break;
            case '0': // what was init_params for 'trainhmm' is now default_params for 'predicthmm'
                int len;
                len = (int)strlen( argv[i] );
                // count delimiters
                n = 1; // start with 1
                for(int j=0;j<len;j++) {
                    n += (argv[i][j]==',')?1:0;
                    if( (argv[i][j] >= 'a' && argv[i][j] <= 'z') || (argv[i][j] >= 'A' && argv[i][j] <= 'Z') ) {
                        strcpy(param.initfile, argv[i]);
                        break;
                    }
                }
                // init parameters
                if(param.init_params!=NULL) free(param.init_params);
                param.init_params = Calloc(NUMBER, (size_t)n);
                param.init_params_n = n;
                // read params and write to params
                param.init_params[0] = atof( strtok(argv[i],",\t\n\r") );
                for(int j=1; j<n; j++) {
                    param.init_params[j] = atof( strtok(NULL,",\t\n\r") );
                }
                break;
			default:
				fprintf(stderr,"unknown option: -%c\n", argv[i-1][1]);
				exit_with_help();
				break;
		}
	}   
    if(param.cv_target_obs>0 && param.metrics>0) { // correct for 0-start coding
        fprintf(stderr,"values for -v and -m cannot be both non-zeros\n");
        exit_with_help();
    }
	
	// next argument should be input (training) file name
	if(i>=argc) // if not
		exit_with_help(); // leave
	
	strcpy(input_file_name, argv[i++]); // copy and advance
	
	if(i>=argc) { // no model file name specified
        fprintf(stderr,"no model file specified\n");
		exit_with_help(); // leave
	}
	else {
		strcpy(model_file_name,argv[i++]); // copy and advance
		if(i>=argc) {// no prediction file name specified
			//strcpy(predict_file_name,"predict_hmm.txt"); // the predict file too
            param.predictions = 0;
        }
		else {
            // param.predictions = 1;
			strcpy(predict_file_name,argv[i]);
        }
	}
}
