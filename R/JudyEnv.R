JudyEnv <- function(){
    e <- new.env(hash=FALSE);
    class(e) <- c(class(e),'UserDefinedDatabase')
    .Call(NewJudyEnv,e);
}
