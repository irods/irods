# N Queens Problem : Place N Queens on an N * N board so that no two queens attack each other.
# The board is represented by a nested list. An empty square is represented by 0, a queen is represented by 1. 
nqueens {
    *board = list(
                  list(0,0,0,0,0,0,0,0),
                  list(0,0,0,0,0,0,0,0),
                  list(0,0,0,0,0,0,0,0),
                  list(0,0,0,0,0,0,0,0),
                  list(0,0,0,0,0,0,0,0),
                  list(0,0,0,0,0,0,0,0),
                  list(0,0,0,0,0,0,0,0),
                  list(0,0,0,0,0,0,0,0)
             );
    tryRow(*board, 0);
}

# Try to place a queen on row *a
tryRow(*board, *a) {
    on(*a == size(*board)) {
        printBoard(*board);
    }
    or {
        trySquare(*board, *a, 0);
    }
}

# Try to place a queen at square (*a, *b)
trySquare(*board, *a, *b) {
    or {
    	accept(*board,*a,*b);
    	tryRow(updateBoard(*board, *a, *b, 1), *a+1);
    }
    on(*b+1 < size(elem(*board, *a))) {
	    trySquare(*board, *a, *b+1);
    }
}

# Test if placing a queen at square (*a, *b) is acceptable.
# Placing a queens at square (*a, *b) is acceptable if it does not attack any queen on the board.
accept(*board, *a, *b) {
    for(*i=0;*i<size(*board);*i=*i+1) {
        if(elem(elem(*board,*i),*b)==1) {
            fail;
        }
    }
    for(*i=0;*i<size(*board);*i=*i+1) {
        *j=*b-*a+*i;
        if(0<=*j && *j<size(elem(*board,*i))){
            if(elem(elem(*board,*i),*j)==1) {
                fail;
            }
        }
        *j=*b+*a-*i;
        if(0<=*j && *j<size(elem(*board,*i))){
            if(elem(elem(*board,*i),*j)==1) {
                fail;
            }
        }
    }
}

# Return a new board that is the same as *board except that square (*a, *b) is *elem  
updateBoard(*board, *a, *b, *elem) = setelem(*board, *a, setelem(elem(*board, *a), *b, *elem));

# Output *board
printBoard(*board) {
    *str = "";
    for(*i=0;*i<size(*board);*i=*i+1) {
	    *row = elem(*board, *i);
        for(*j=0;*j<size(*row);*j=*j+1) {
            *str = *str ++ str(elem(*row, *j)) ++ " ";
        }
        *str = *str ++ "\n";
    }
    writeLine("stdout", *str);
}

input null
output ruleExecOut

