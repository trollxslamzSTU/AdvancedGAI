#pragma once

#include "ChessPlayer.h"
#include "ChessCommons.h"
#include <SDL.h>

class ChessPlayerAI : public ChessPlayer
{
//--------------------------------------------------------------------------------------------------
public:
	ChessPlayerAI(sdl_game::app_context & context, COLOUR colour, Board* board, vector<SDL_Point>* highlights, SDL_Point* selectedPiecePosition, Move* lastMove, int* searchDepth);
	~ChessPlayerAI();

	bool		TakeATurn(SDL_Event e);

//--------------------------------------------------------------------------------------------------
protected:
	int  MiniMax(Board board, int depth, Move* bestMove);
	int  Maximise(Board board, int depth, Move* bestMove, int alphaBeta);
	int  Minimise(Board board, int depth, Move* bestMove, int alphaBeta);
	bool MakeAMove(Move* move, Board* board);
	void UnMakeAMove(Move move, Board currentBoard);

	void OrderMoves(Board board, vector<Move>* moves, bool highToLow);
	void ValueMoves(Board board, vector<Move>* moves);
	void CropMoves(vector<Move>* moves, unsigned int maxNumberOfMoves);

	int  ScoreTheBoard(Board boardToScore);
	int	 ScoreBoardPieces(Board boardToScore);
	int  ScoreBoardPositioning(Board boardToScore);

	
	bool IsGameOver(Board boardToCheck);

private:
	int* mDepthToSearch;
	vector<Move> moves;
	Move mBestMove;

	COLOUR mOpponentColour;
};
