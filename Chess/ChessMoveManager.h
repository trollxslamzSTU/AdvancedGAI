//------------------------------------------------------------------------
//  Author: Paul Roberts 2016
//------------------------------------------------------------------------

#ifndef _CHESS_MOVEMANAGER_H
#define _CHESS_MOVEMANAGER_H

#include "sdl_game.h"
#include "ChessCommons.h"
#include <vector>
using namespace std;

//--------------------------------------------------------------------------------------------------

class MoveManager
{
//--------------------------------------------------------------------------------------------------
public:
	~MoveManager();

	static MoveManager* Instance();

	void ClearRecordedMoves();

	void StoreMove(SDL_Point fromBoardPosition, SDL_Point toBoardPosition);
	void StoreMove(Move move);

	bool HasRecordedMoves();
	Move GetLastMove();

	void OutputGameState(GAMESTATE gameState, COLOUR playerTurn);

//--------------------------------------------------------------------------------------------------
private:
	MoveManager();
	string ConvertBoardPositionIntToLetter(int numericalValue);
	void   OutputMove(ChessMove move);

//--------------------------------------------------------------------------------------------------
private:
	static MoveManager* mInstance;

	vector<ChessMove> mRecordedChessMoves;
};

//--------------------------------------------------------------------------------------------------

#endif //_CHESS_MOVEMANAGER_H
