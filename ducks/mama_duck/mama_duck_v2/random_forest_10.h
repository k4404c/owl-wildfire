#pragma once
#include <cstdarg>
#include <cstdint>
namespace Eloquent {
    namespace ML {
        namespace Port {
            class RandomForest {
                public:
                    /**
                    * Predict class for features vector
                    */
                    int predict(float *x) {
                        uint8_t votes[2] = { 0 };
                        // tree #1
                        if (x[2] <= -0.9384118616580963) {
                            if (x[5] <= 0.036514824256300926) {
                                if (x[4] <= 1.0992498099803925) {
                                    votes[0] += 1;
                                }

                                else {
                                    votes[1] += 1;
                                }
                            }

                            else {
                                votes[1] += 1;
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #2
                        if (x[2] <= -0.9384118616580963) {
                            if (x[1] <= -0.3601927012205124) {
                                votes[1] += 1;
                            }

                            else {
                                if (x[6] <= 0.006665616761893034) {
                                    if (x[8] <= -0.0035800085752271116) {
                                        votes[1] += 1;
                                    }

                                    else {
                                        votes[0] += 1;
                                    }
                                }

                                else {
                                    votes[1] += 1;
                                }
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #3
                        if (x[4] <= 0.5790334641933441) {
                            if (x[2] <= -0.7425177693367004) {
                                if (x[6] <= 0.004250231897458434) {
                                    votes[0] += 1;
                                }

                                else {
                                    votes[1] += 1;
                                }
                            }

                            else {
                                votes[1] += 1;
                            }
                        }

                        else {
                            if (x[2] <= -1.036358892917633) {
                                if (x[7] <= -0.014621875830926001) {
                                    votes[1] += 1;
                                }

                                else {
                                    votes[0] += 1;
                                }
                            }

                            else {
                                if (x[0] <= -0.2309324312955141) {
                                    if (x[4] <= 1.2661281526088715) {
                                        votes[0] += 1;
                                    }

                                    else {
                                        votes[1] += 1;
                                    }
                                }

                                else {
                                    votes[1] += 1;
                                }
                            }
                        }

                        // tree #4
                        if (x[5] <= 0.014906374737620354) {
                            votes[0] += 1;
                        }

                        else {
                            if (x[0] <= -1.7239412665367126) {
                                if (x[6] <= -0.08184515102766454) {
                                    votes[1] += 1;
                                }

                                else {
                                    votes[0] += 1;
                                }
                            }

                            else {
                                if (x[4] <= 0.7656183838844299) {
                                    votes[1] += 1;
                                }

                                else {
                                    if (x[5] <= 0.016832378692924976) {
                                        if (x[4] <= 1.6603617370128632) {
                                            votes[0] += 1;
                                        }

                                        else {
                                            votes[1] += 1;
                                        }
                                    }

                                    else {
                                        votes[1] += 1;
                                    }
                                }
                            }
                        }

                        // tree #5
                        if (x[6] <= 0.004833422135561705) {
                            if (x[5] <= 0.014959174208343029) {
                                votes[0] += 1;
                            }

                            else {
                                if (x[3] <= 0.15838249772787094) {
                                    votes[0] += 1;
                                }

                                else {
                                    if (x[1] <= -0.1804233193397522) {
                                        votes[1] += 1;
                                    }

                                    else {
                                        if (x[5] <= 0.018944271840155125) {
                                            votes[0] += 1;
                                        }

                                        else {
                                            votes[1] += 1;
                                        }
                                    }
                                }
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #6
                        if (x[2] <= -0.9384118616580963) {
                            if (x[4] <= 1.3473059833049774) {
                                if (x[5] <= 0.037621909752488136) {
                                    votes[0] += 1;
                                }

                                else {
                                    votes[1] += 1;
                                }
                            }

                            else {
                                votes[1] += 1;
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #7
                        if (x[5] <= 0.014906374737620354) {
                            votes[0] += 1;
                        }

                        else {
                            if (x[0] <= -1.5279226303100586) {
                                if (x[2] <= -0.6445707380771637) {
                                    votes[0] += 1;
                                }

                                else {
                                    votes[1] += 1;
                                }
                            }

                            else {
                                if (x[0] <= 0.12190117686986923) {
                                    if (x[4] <= 1.0817658603191376) {
                                        if (x[6] <= -0.0011667056242004037) {
                                            votes[0] += 1;
                                        }

                                        else {
                                            votes[1] += 1;
                                        }
                                    }

                                    else {
                                        votes[1] += 1;
                                    }
                                }

                                else {
                                    votes[1] += 1;
                                }
                            }
                        }

                        // tree #8
                        if (x[0] <= -1.4005104899406433) {
                            if (x[3] <= 0.7733395248651505) {
                                votes[0] += 1;
                            }

                            else {
                                votes[1] += 1;
                            }
                        }

                        else {
                            if (x[6] <= -0.004000072251074016) {
                                if (x[7] <= -0.001516676158644259) {
                                    votes[0] += 1;
                                }

                                else {
                                    if (x[0] <= 0.18724073469638824) {
                                        if (x[5] <= 0.034767745062708855) {
                                            votes[0] += 1;
                                        }

                                        else {
                                            votes[1] += 1;
                                        }
                                    }

                                    else {
                                        votes[1] += 1;
                                    }
                                }
                            }

                            else {
                                votes[1] += 1;
                            }
                        }

                        // tree #9
                        if (x[2] <= -0.9384118616580963) {
                            if (x[2] <= -1.1343059539794922) {
                                if (x[3] <= 0.48948897421360016) {
                                    votes[0] += 1;
                                }

                                else {
                                    votes[1] += 1;
                                }
                            }

                            else {
                                if (x[4] <= 0.5215187966823578) {
                                    if (x[4] <= 0.2518381178379059) {
                                        votes[0] += 1;
                                    }

                                    else {
                                        votes[1] += 1;
                                    }
                                }

                                else {
                                    votes[0] += 1;
                                }
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // tree #10
                        if (x[0] <= -0.11005425080657005) {
                            if (x[5] <= 0.0626208083704114) {
                                votes[0] += 1;
                            }

                            else {
                                votes[1] += 1;
                            }
                        }

                        else {
                            votes[1] += 1;
                        }

                        // return argmax of votes
                        uint8_t classIdx = 0;
                        float maxVotes = votes[0];

                        for (uint8_t i = 1; i < 2; i++) {
                            if (votes[i] > maxVotes) {
                                classIdx = i;
                                maxVotes = votes[i];
                            }
                        }

                        return classIdx;
                    }

                protected:
                };
            }
        }
    }