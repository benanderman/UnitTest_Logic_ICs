# UnitTest_Logic_ICs
Arduino unit tests for 74-series logic chips. Currently supports gates (00, 02, 04, 08, 30, 32, 86), other stateless chips (139, 157, 283), the 161 4-bit counter, and 173 4-bit register. Sets all possible input values for stateless chips, and verifies the outputs are correct. For chips that store a state, tests a lot of the possible combinations of inputs and functionality.
