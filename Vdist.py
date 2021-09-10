import random

random.seed(1337)

for Si in range(1000):
  for i in range(5):
    vi = random.random()*(1.+Si/100.)
    Vi = 2**vi
    print(Si, Vi)
