# UT_diss_code

Code base for dissertation titled "A Game Theoretic Approach to Nuclear Safeguards Selection and Optimization."

Most current code is in the fullmodel directory. Use full_fp.c to run the model. The user should alter the budget, convergence criteria (vgap), and attacker risk tolerance (alpha) as desired. Note that decreasing vgap will increase run time.

directories:
fullmodel: has combined model with both enrichment and reprocessing plants.
  - full_fp.c has the full simulation model and the fictitious play algorithm
    to optimize defender and attacker strategies.
  - full_sim.c is just the combined reprocessing and simulation models. For a
    given attacker and defender strategy pair, you can calculate a detection
    probability.
  - fullmodel_it.c uses a set number of iterations to drive the fictitious
    play algorithm, rather than running until a convergence criteria is met.
  - calcPayoff.c calculates the payoff for a given attacker-defender strategy
    pair.
