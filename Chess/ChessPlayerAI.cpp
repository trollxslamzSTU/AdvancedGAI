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

const int kPawnScore		= 100;
const int kKnightScore		= 320;
const int kBishopScore		= 330;
const int kRookScore		= 500;
const int kQueenScore		= 900;
const int kKingScore		= 20000;

const int kCheckScore		= 1;
const int kCheckmateScore	= 1;
const int kStalemateScore	= 1;	//Tricky one because sometimes you want this, sometimes you don't.

const int kPieceWeight		= 1; //Scores as above.
const int kMoveWeight		= 1; //Number of moves available to pieces.
const int kPositionalWeight	= 1; //Whether in CHECK, CHECKMATE or STALEMATE.

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
	MiniMax(*mChessBoard, *mDepthToSearch, moves.data());
	bool gameStillActive = MakeAMove(&mBestMove, mChessBoard);

	return gameStillActive;
	//-----------------------------------------------------------
}

//--------------------------------------------------------------------------------------------------

int ChessPlayerAI::MiniMax(Board board, int depth, Move* currentMove)
{
	return Maximise(board, depth, currentMove, INT_MAX);
}

//--------------------------------------------------------------------------------------------------

int ChessPlayerAI::Maximise(Board board, int depth, Move* currentMove, int parentLow)
{
	//TODO
	
	if (depth == 0 || IsGameOver(board))
	{
		return ScoreTheBoard(board);
	}

	int max = -INT_MIN;
	vector<Move> tempMoves;
	GetAllMoveOptions(board, mTeamColour, &tempMoves);

	for (Move& move : tempMoves)
	{
		Board boardCopy;
		boardCopy = board;
		MakeAMove(&move, &boardCopy);
		int maxEval = Minimise(boardCopy, depth - 1, currentMove, max);
		if (maxEval > max)
		{
			max = maxEval;
			if (depth == *mDepthToSearch)
			{
				mBestMove = move;
			}
		}
		
	};
		
	
	return max;
}

//--------------------------------------------------------------------------------------------------

int ChessPlayerAI::Minimise(Board board, int depth, Move* bestMove, int parentHigh)
{
	//TODO
	
	if (depth == 0 || IsGameOver(board))
	{
		return ScoreTheBoard(board);
	}

	int min = INT_MAX;
	vector <Move> tempMoves;
	GetAllMoveOptions(board, mOpponentColour, &tempMoves);

	for (Move& move : tempMoves)
	{
		Board boardCopy;
		boardCopy = board;
		MakeAMove(&move, &boardCopy);
		int minEval = Maximise(boardCopy, depth - 1, bestMove, min);
		if (minEval < min)
		{
			min = minEval;
			if (depth == *mDepthToSearch)
			{
				mBestMove = move;
			}
		}
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
	for (Move& move : *moves)
	{
		BoardPiece piece = board.currentLayout[move.from_X][move.from_Y];
		int pieceValue = 0;
		switch (piece.piece)
		{
		case PIECE_PAWN:
			pieceValue = kPawnScore / 5;
			break;

		case PIECE_KNIGHT:
			pieceValue = kKnightScore / 5;
			break;

		case PIECE_BISHOP:
			pieceValue = kBishopScore / 5;
			break;

		case PIECE_ROOK:
			pieceValue = kRookScore / 5;
			break;

		case PIECE_QUEEN:
			pieceValue = kQueenScore / 5;
			break;

		case PIECE_KING:
			pieceValue = kQueenScore / 5;
			break;

		case PIECE_NONE:
			break;

		default:
			pieceValue = 0;
			break;
		}
		BoardPiece capPiece = board.currentLayout[move.to_X][move.to_Y];
		if (capPiece.piece != PIECE_NONE)
		{
			int capPieceValue = 0;
			switch (piece.piece)
			{
			case PIECE_PAWN:
				capPieceValue = kPawnScore / 5;
				break;

			case PIECE_KNIGHT:
				capPieceValue = kKnightScore / 5;
				break;

			case PIECE_BISHOP:
				capPieceValue = kBishopScore / 5;
				break;

			case PIECE_ROOK:
				capPieceValue = kRookScore / 5;
				break;

			case PIECE_QUEEN:
				capPieceValue = kQueenScore / 5;
				break;

			case PIECE_KING:
				capPieceValue = kQueenScore / 5;
				break;

			case PIECE_NONE:
				break;

			default:
				capPieceValue = 0;
				break;
			}
			pieceValue += capPieceValue;

		}


		move.score = pieceValue;
	}
}

//--------------------------------------------------------------------------------------------------

void ChessPlayerAI::CropMoves(vector<Move>* moves, unsigned int maxNumberOfMoves)
{
	//TODO
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
					total = total + kPawnScore;
				}
				else
				{
					total = total - kPawnScore;
				}

				break;

			case PIECE_KNIGHT:
				if (currentPiece.colour == mTeamColour)
				{
					total = total + kKnightScore;
				}
				else
				{
					total = total - kKnightScore;
				}
					
				break;

			case PIECE_BISHOP:
				if (currentPiece.colour == mTeamColour)
				{
					total = total + kBishopScore;
				}
				else
				{
					total = total - kBishopScore;
				}

				break;

			case PIECE_ROOK:
				if (currentPiece.colour == mTeamColour)
				{
					total = total + kRookScore;
				}
				else
				{
					total = total - kRookScore;
				}

				break;

			case PIECE_QUEEN:
				if (currentPiece.colour == mTeamColour)
				{
					total = total + kQueenScore;
				}
				else
				{
					total = total - kQueenScore;
				}

				break;

			case PIECE_KING:
				if (currentPiece.colour == mTeamColour)
				{
					total = total + kKingScore;
				}
				else
				{
					total = total - kKingScore;
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
			if ((x == 3 || x == 4) && (y == 3 || y == 4))
			{
				BoardPiece currentPiece = boardToScore.currentLayout[x][y];
				if (currentPiece.piece != PIECE_NONE && currentPiece.colour == mTeamColour)
				{
					total = total + 100;
				}
				if (currentPiece.piece != PIECE_NONE && currentPiece.colour == mOpponentColour)
				{
					total = total - 100;
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


//--------------------------------------------------------------------------------------------------
