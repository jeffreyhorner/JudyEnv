# BENCHMARK FILE.
# - Demo file for comparing hash benchmarks.
# 

library(JudyEnv)
library(microbenchmark)
#library(ggplot2)

# STEP 0. CREATE A SAMPLE SET OF KEYS AND VALUES.  
#   size: the sample size
#   keys:   hash's keys
#   values: hash's values

size   <- 2^20   # The size of the refernece objects. 
keys   <- as.character( sample(1:size) )  # A vector of
values <- rnorm( size )

# Which is faster setting by mapply or doing a for-loop
# Intialize parameters and prepare things.

# ---------------------------------------------------------------------
# BENCHMARK 2: ACCESSING SINGLE VALUES 
#   Compare times for accessing single elements of a list vs vector vs hash 
#
# CONCLUSIONS:
#  - For number of items, looking up in a list is faster than looking 
#    up in an environment. 
#  
# ---------------------------------------------------------------------

# Create a list using mapply, n.b much faster than for-loop
message( "BENCHMARK 2: Accessing a single value in a large hash structure\n" )

# LOOP OVER SIX ORDERS OF MAGNITUDES.
for( size in 2^(10:15) ) {

  cat( "\nComparing access time for",size,"objects\n" )

  # native env
  e <- new.env()

  J <- JudyEnv()

  print(microbenchmark(
    `nativecons` = for (i in 1:size) assign(keys[i],values[i],envir=e),
    `JudyEncons` = for (i in 1:size) assign(keys[i],values[i],envir=J)
  ))

  # CREATE KEYS TO LOOK UP:
  ke <- keys[ round(runif(max=size,min=1,n=size )) ]
  
  print(microbenchmark(
    `nativelook` = for (k in ke) y = get(k,envir=e),
    `JudyEnlook` = for (k in ke) y = get(k,envir=J)
  ))

}
