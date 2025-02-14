//------------------------------------------------------------------------
//  Author: Paul Roberts 2016
//------------------------------------------------------------------------

#include "ChessPlayerAI.h"
#include <iostream>		//cout
#include <SDL.h>
#include <iomanip>		//Precision
#include <algorithm>	//Sort
#include "ChessConstants.h"
#include "ChessMoveManager.h"

using namespace::std;

//--------------------------------------------------------------------------------------------------

//TODO
//Change these values as you see fit, add or remove values

const int kPawnScore		= 200;
const int kKnightScore		= 400;
const int kBishopScore		= 1000;
const int kRookScore		= 1200;
const int kQueenScore		= 2000;
const int kKingScore		= 20000;

const int kCheckScore		= 1;
const int kCheckmateScore	= 1;
const int kStalemateScore	= 1;	//Tricky one because sometimes you want this, sometimes you don't.

const int kPieceWeight		= 1; //Scores as above.
const int kMoveWeight		= 1; //Number of moves available to pieces.
const int kPositionalWeight	= 1; //Whether in CHECK, CHECKMATE or STALEMATE.
const int kOrderWieght = 3;
const int kScoreWeight = 2;
const int kSquareWeight = 125;
int MVVLVA[6][6] = {
	
	{ 105, 205, 305, 405, 505, 1005 }, 
	{ 104, 204, 304, 404, 504, 1004 }, 
	{ 103, 203, 303, 403, 503, 1003 }, 
	{ 102, 202, 302, 402, 502, 1002 }, 
	{ 101, 201, 301, 401, 501, 1001 }, 
	{ 100, 200, 300, 400, 500, 1000 }  
};
//--------------------------------------------------------------------------------------------------

ChessPlayerAI::ChessPlayerAI(sdl_game::app_context & context, COLOUR colour, Board* board, vector<SDL_Point>* highlights, SDL_Point* selectedPiecePosition, Move* lastMove, int* searchDepth)
	: ChessPlayer(context, colour, board, highlights, selectedPiecePosition, lastMove)
{
	mDepthToSearch = searchDepth;

	if (colour == COLOUR_WHITE)
	{
		mOpponentColour = COLOUR_BLACK;
	}
	else if (colour == COLOUR_BLACK)
	{
		mOpponentColour = COLOUR_WHITE;
	}
}

//--------------------------------------------------------------------------------------------------

ChessPlayerAI::~ChessPlayerAI()
{
}

//--------------------------------------------------------------------------------------------------

bool ChessPlayerAI::MakeAMove(Move* move, Board* chessBoard)
{
	//If this was an en'passant move the taken piece will not be in the square we moved to.
	if(chessBoard->currentLayout[move->from_X][move->from_Y].piece == PIECE_PAWN )
	{
		//If the pawn is on its start position and it double jumps, then en'passant may be available for opponent.
		if( (move->from_Y == 1 && move->to_Y == 3) ||
			(move->from_Y == 6 && move->to_Y == 4) )
		{
			chessBoard->currentLayout[move->from_X][move->from_Y].canEnPassant = true;
		}
	}

	//En'Passant removal of enemy pawn.
	//If our pawn moved into an empty position to the left or right, then must be En'Passant.
	if(chessBoard->currentLayout[move->from_X][move->from_Y].piece == PIECE_PAWN &&
		chessBoard->currentLayout[move->to_X][move->to_Y].piece == PIECE_NONE )
	{
		int pawnDirectionOpposite = mTeamColour == COLOUR_WHITE ? -1 : 1;

		if( (move->from_X < move->to_X) ||
			(move->from_X > move->to_X) )
		{
			chessBoard->currentLayout[move->to_X][move->to_Y+pawnDirectionOpposite] = BoardPiece();
		}
	}

	//CASTLING - Move the rook.
	if(chessBoard->currentLayout[move->from_X][move->from_Y].piece == PIECE_KING )
	{
		//Are we moving 2 spaces??? This indicates CASTLING.
		if( move->to_X-move->from_X == 2 )
		{
			//Moving 2 spaces to the right - Move the ROOK on the right into its new position.
			chessBoard->currentLayout[move->from_X+3][move->from_Y].hasMoved = true;
			chessBoard->currentLayout[move->from_X+1][move->from_Y] = chessBoard->currentLayout[move->from_X+3][move->from_Y];
			chessBoard->currentLayout[move->from_X+3][move->from_Y] = BoardPiece();
		}
		else if( move->to_X-move->from_X == -2 )
		{
			//Moving 2 spaces to the left - Move the ROOK on the left into its new position.
			//Move the piece into new position.
			chessBoard->currentLayout[move->from_X-4][move->from_Y].hasMoved = true;
			chessBoard->currentLayout[move->from_X-1][move->from_Y] = chessBoard->currentLayout[move->from_X-4][move->from_Y];
			chessBoard->currentLayout[move->from_X-4][move->from_Y] = BoardPiece();
		}
	}

	//Move the piece into new position.
	chessBoard->currentLayout[move->from_X][move->from_Y].hasMoved = true;
	chessBoard->currentLayout[move->to_X][move->to_Y] = chessBoard->currentLayout[move->from_X][move->from_Y];
	chessBoard->currentLayout[move->from_X][move->from_Y] = BoardPiece();

	//Store the last move to output at start of turn.
	mLastMove->from_X = move->from_X;
	mLastMove->from_Y = move->from_Y;
	mLastMove->to_X = move->to_X;
	mLastMove->to_Y = move->to_Y;

	//Record the move.
	MoveManager::Instance()->StoreMove(*move);

	//Piece is in a new position.
	mSelectedPiecePosition->x = move->to_X;
	mSelectedPiecePosition->y = move->to_Y;

	//Check if we need to promote a pawn.
	if(chessBoard->currentLayout[move->to_X][move->to_Y].piece == PIECE_PAWN && (move->to_Y == 0 || move->to_Y == 7) )
	{
		//Time to promote - Always QUEEN at the moment.
		PIECE newPieceType  = PIECE_QUEEN;

		//Change the PAWN into the selected piece.
		chessBoard->currentLayout[move->to_X][move->to_Y].piece = newPieceType;
	}

	//Not finished turn yet.
	return true;
}

//--------------------------------------------------------------------------------------------------

bool ChessPlayerAI::TakeATurn(SDL_Event e)
{
	//TODO: Code your own function - Remove this version after, it is only here to keep the game functioning for testing.
	GetAllMoveOptions(*mChessBoard, mTeamColour, &moves);
	OrderMoves(*mChessBoard, &moves, true);
	CropMoves(&moves, 10);
	MiniMax(*mChessBoard, *mDepthToSearch, moves.data());
	bool gameStillActive = MakeAMove(&mBestMove, mChessBoard);

	return gameStillActive;
	//-----------------------------------------------------------
}

//--------------------------------------------------------------------------------------------------

int ChessPlayerAI::MiniMax(Board board, int depth, Move* currentMove)
{
	return Maximise(board, depth, currentMove, -INT_MAX, INT_MAX);
}

//--------------------------------------------------------------------------------------------------

int ChessPlayerAI::Maximise(Board board, int depth, Move* currentMove, int alpha, int beta)
{
	//TODO
	
	if (depth == 0 || IsGameOver(board))
	{
		return ScoreTheBoard(board);
	}

	int max = INT_MIN;
	
	vector<Move> tempMoves;
	GetAllMoveOptions(board, mTeamColour, &tempMoves);
	OrderMoves(board, &tempMoves, false);
	CropMoves(&moves, 5);

	for (Move& move : tempMoves)
	{
		Board boardCopy;
		boardCopy = board;
		MakeAMove(&move, &boardCopy);
		int maxEval = Minimise(boardCopy, depth - 1, currentMove, alpha, beta);
		if (maxEval > max)
		{
			max = alpha;
			if (maxEval > alpha)
			{
				alpha = maxEval;
			}
			if (depth == *mDepthToSearch)
			{
				mBestMove = move;
			}
		}
		if (maxEval >= beta) return maxEval;
	}
	return max;
}

//--------------------------------------------------------------------------------------------------

int ChessPlayerAI::Minimise(Board board, int depth, Move* bestMove, int alpha , int beta)
{
	//TODO
	
	if (depth == 0 || IsGameOver(board))
	{
		return ScoreTheBoard(board) * -1;
	}

	int min = INT_MAX;
	
	vector <Move> tempMoves;
	GetAllMoveOptions(board, mOpponentColour, &tempMoves);
	OrderMoves(board, &tempMoves, true);
	CropMoves(&moves, 5);
	for (Move& move : tempMoves)
	{
		Board boardCopy;
		boardCopy = board;
		MakeAMove(&move, &boardCopy);
		int minEval = Maximise(boardCopy, depth - 1, bestMove, alpha, beta);
		if (minEval < min)
		{
			min = minEval;
			if (minEval < beta)
			{
				beta = minEval;
			}
			if (depth == *mDepthToSearch)
			{
				mBestMove = move;
			}
		}
		if (minEval <= alpha) return minEval;	
	}
	return min;
}


void ChessPlayerAI::UnMakeAMove(Move move, Board currentBoard)
{
	Move *mUnmake = new Move(move.to_X, move.to_Y, move.from_X, move.from_Y);
	MakeAMove(mUnmake, &currentBoard);
}

//--------------------------------------------------------------------------------------------------

void ChessPlayerAI::OrderMoves(Board board, vector<Move>* moves, bool highToLow)
{
	////TODO
	ValueMoves(board, moves);
	if (highToLow)
	{
		std::sort(moves->begin(), moves->end(), [](Move a, Move b)
			{
				return a.score > b.score;
			});
	}
	else 
	{
		std::sort(moves->begin(), moves->end(), [](Move a, Move b)
			{
				return a.score < b.score;
			});

	}
}

void ChessPlayerAI::ValueMoves(Board board, vector<Move>* moves)
{
	int moveValue;
	moveValue = 0;
	for (Move& move : *moves)
	{	
		BoardPiece capPiece = board.currentLayout[move.to_X][move.to_Y];
		BoardPiece attackerPiece = board.currentLayout[move.from_X][move.from_Y];
		if (capPiece.piece != PIECE_NONE)
		{
			moveValue += MVVLVA[GetPieceIndex(capPiece.piece)][GetPieceIndex(attackerPiece.piece)];
		}
		move.score = moveValue;
		/*BoardPiece capPiece = board.currentLayout[move.to_X][move.to_Y];
		if (capPiece.piece != PIECE_NONE)
		{
			int capPieceValue = 0;
			switch (capPiece.piece)
			{
			case PIECE_PAWN:
				capPieceValue = kPawnScore / kOrderWieght;
				break;

			case PIECE_KNIGHT:
				capPieceValue = kKnightScore / kOrderWieght;
				break;

			case PIECE_BISHOP:
				capPieceValue = kBishopScore / kOrderWieght;
				break;

			case PIECE_ROOK:
				capPieceValue = kRookScore / kOrderWieght;
				break;

			case PIECE_QUEEN:
				capPieceValue = kQueenScore / kOrderWieght;
				break;

			case PIECE_KING:
				capPieceValue = kQueenScore / kOrderWieght;
				break;

			case PIECE_NONE:
				break;

			default:
				capPieceValue = 0;
				break;
			}
			moveValue += capPieceValue;
		}
		BoardPiece piece = board.currentLayout[move.from_X][move.from_Y];
		if (piece.piece != PIECE_NONE)
		{
			int pieceValue = 0;
			switch (piece.piece)
			{
			case PIECE_PAWN:
				pieceValue = kPawnScore / kOrderWieght;
				break;

			case PIECE_KNIGHT:
				pieceValue = kKnightScore / kOrderWieght;
				break;

			case PIECE_BISHOP:
				pieceValue = kBishopScore / kOrderWieght;
				break;

			case PIECE_ROOK:
				pieceValue = kRookScore / kOrderWieght;
				break;

			case PIECE_QUEEN:
				pieceValue = kQueenScore / kOrderWieght;
				break;

			case PIECE_KING:
				pieceValue = kQueenScore / kOrderWieght;
				break;

			case PIECE_NONE:
				break;

			default:
				pieceValue = 0;
				break;
			}
			moveValue += pieceValue;
		}
		move.score = moveValue;*/
	}
}

int ChessPlayerAI::GetPieceIndex(PIECE piece)
{
	switch (piece)
	{
	case PIECE_PAWN:
		return 0;
	case PIECE_KNIGHT:
		return 1;
	case PIECE_BISHOP:
		return 2;
	case PIECE_ROOK:
		return 3;
	case PIECE_QUEEN:
		return 4;
	case PIECE_KING:
		return 5;
	default:
		return 0; 
	}
}
//--------------------------------------------------------------------------------------------------

void ChessPlayerAI::CropMoves(vector<Move>* moves, unsigned int maxNumberOfMoves)
{
	if (moves->size() > maxNumberOfMoves)
	{
		std::sort(moves->begin(), moves->end(), [](Move a, Move b)
			{
				return a.score > b.score;
			});

		moves->resize(maxNumberOfMoves);
	}
}

//--------------------------------------------------------------------------------------------------

int ChessPlayerAI::ScoreTheBoard(Board boardToScore)
{
	int OverallTotal = 0;
	OverallTotal = ScoreBoardPieces(boardToScore) + ScoreBoardPositioning(boardToScore);
	return OverallTotal;
}

int ChessPlayerAI::ScoreBoardPieces(Board boardToScore)
{
	int total = 0;
	for (int x = 0; x < 8; x++)
	{
		for (int y = 0; y < 8; y++)
		{
			BoardPiece currentPiece = boardToScore.currentLayout[x][y];
			switch (currentPiece.piece)
			{
			case PIECE_PAWN:
				if (currentPiece.colour == mTeamColour)
				{
					total = total + kPawnScore * kScoreWeight;
				}
				else
				{
					total = total - kPawnScore * kScoreWeight;
				}

				break;

			case PIECE_KNIGHT:
				if (currentPiece.colour == mTeamColour)
				{
					total = total + kKnightScore * kScoreWeight;
				}
				else
				{
					total = total - kKnightScore * kScoreWeight;
				}
					
				break;

			case PIECE_BISHOP:
				if (currentPiece.colour == mTeamColour)
				{
					total = total + kBishopScore * kScoreWeight;
				}
				else
				{
					total = total - kBishopScore * kScoreWeight;
				}

				break;

			case PIECE_ROOK:
				if (currentPiece.colour == mTeamColour)
				{
					total = total + kRookScore * kScoreWeight;
				}
				else
				{
					total = total - kRookScore * kScoreWeight;
				}

				break;

			case PIECE_QUEEN:
				if (currentPiece.colour == mTeamColour)
				{
					total = total + kQueenScore * kScoreWeight;
				}
				else
				{
					total = total - kQueenScore * kScoreWeight;
				}

				break;

			case PIECE_KING:
				if (currentPiece.colour == mTeamColour)
				{
					total = total + kKingScore * kScoreWeight;
				}
				else
				{
					total = total - kKingScore * kScoreWeight;
				}

				break;

			case PIECE_NONE:
				break;

			default:
				break;
			}
		}
	}
	return total;
}

int ChessPlayerAI::ScoreBoardPositioning(Board boardToScore)
{
	int total = 0;
	for (int x = 0; x < 8; x++)
	{
		for (int y = 0; y < 8; y++)
		{
			if (x == 3 || x == 4)
			{
				BoardPiece currentPiece = boardToScore.currentLayout[x][y];
				if (currentPiece.piece != PIECE_NONE && currentPiece.colour == mTeamColour)
				{
					total = total + kSquareWeight;
				}
				if (currentPiece.piece != PIECE_NONE && currentPiece.colour == mOpponentColour)
				{
					total = total - kSquareWeight;
				}
			}
		}
	}
	return total;
}



bool ChessPlayerAI::IsGameOver(Board boardToCheck)
{
	mInCheck = CheckForCheck(boardToCheck, mTeamColour);
	if (mInCheck)
	{
		if (CheckForCheckmate(boardToCheck, mTeamColour)) return true;	
	}
	if (!mInCheck)
	{
		if (CheckForStalemate(boardToCheck, mTeamColour)) return true;
	}
	return false;
}


//--------------------------------------------------------------------------------------------------//