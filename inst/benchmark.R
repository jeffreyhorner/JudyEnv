# BENCHMARK FILE.
# - Demo file for comparing hash benchmarks.
# 

library(hash)
library(JudyEnv)
library(microbenchmark)
#library(ggplot2)

# STEP 0. CREATE A SAMPLE SET OF KEYS AND VALUES.  
#   size: the sample size
#   keys:   hash's keys
#   values: hash's values

size   <- 1e4   # The size of the refernece objects. 
keys   <- as.character( sample(1:size) )  # A vector of
values <- as.character( rnorm( size ) )

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

number.of.lookups <- 1e3
bm2 <- data.frame() 


# LOOP OVER SIX ORDERS OF MAGNITUDES.
for( size in 2^(1:13) ) {

  cat( "\nComparing access time for object of size", size, "\n" )

  # CREATE NAMED-HASH:
  ha <- hash( keys[1:size], values[1:size] )

  # JudyEnv 
  je <- JudyEnv()
  for (i in 1:size){
    assign(keys[i],values[i],envir=je)
  }

  # native env
  e <- new.env()
  for (i in 1:size){
    assign(keys[i],values[i],envir=e)
  }

  # CREATE KEYS TO LOOK UP:
  ke <- keys[ round(runif(max=size,min=1,n=number.of.lookups )) ]
  
  print(
  res <-  microbenchmark( 
      `get/hash`   = for( k in ke ) if(get(k,envir=e)!=get(k, e)) stop('get/hash') ,
      `hash`  = for( k in ke ) if (ha[[k]] != get(k, e)) stop('hash'),
      `get/judyenv` = for (k in ke) if (get(k,envir=je) != get(k, e)) stop ('get/judyenv'),
      `get/native` = for (k in ke) if (get(k,envir=e)  != get(k, e)) stop('get/native')
    )
  ) 
#  res$size <- size
#  bm2 <- rbind( bm2, res )   
#    autoplot(res)

}
#x11()
#xyplot( 
#  elapsed ~ size, groups=test, 
#  data=bm2, 
#  type="b", pch=16:20, col=rainbow(5), 
#  lwd=2, main="Reading from data structures", cex=1.2, cex.title=4, 
#  auto.key=list(space = "right", points = FALSE, lines = FALSE, lwd=4, cex=1, col=rainbow(5)) ,
#  scales=list( cex=2 ), 
#  ylab = "Elapsed Time ( per 1K Reads)" , 
#  xlab = "Object Size ( n elements )" 
#)  
