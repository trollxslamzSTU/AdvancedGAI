//------------------------------------------------------------------------
//  Author: Paul Roberts 2016
//------------------------------------------------------------------------

#ifndef _CHESSPLAYER_H
#define _CHESSPLAYER_H

#include "ChessCommons.h"
#include <SDL.h>
#include <vector>
using namespace::std;

class Texture2D;

class ChessPlayer
{
//--------------------------------------------------------------------------------------------------
public:
	ChessPlayer(sdl_game::app_context & context, COLOUR colour, Board* board, vector<SDL_Point>* highlights, SDL_Point* selectedPiecePosition, Move* lastMove);
	~ChessPlayer();

	COLOUR	 GetColour()			{return mTeamColour;}
	MOVETYPE GetMoveType()			{return mCurrentMove;}

//--------------------------------------------------------------------------------------------------
public:
	virtual GAMESTATE	PreTurn();
	virtual bool		TakeATurn(SDL_Event e);
	virtual void		EndTurn();

	void				RenderPawnPromotion(sdl_game::app_context & context);

//--------------------------------------------------------------------------------------------------
protected:
	virtual bool MakeAMove(SDL_Point boardPosition);

	void GetNumberOfLivingPieces();
	void GetMoveOptions(SDL_Point piecePosition, BoardPiece boardPiece, Board boardToTest, vector<Move>* moves);
	void GetAllMoveOptions(Board boardToTest, COLOUR teamColour, vector<Move>* moves);
	bool CheckMoveOptionValidityAndStoreMove(Move moveToCheck, COLOUR pieceColour, Board boardToTest, vector<Move>* moves);

	void GetPawnMoveOptions(SDL_Point piecePosition, BoardPiece boardPiece, Board boardToTest, vector<Move>* moves);
	void GetHorizontalAndVerticalMoveOptions(SDL_Point piecePosition, BoardPiece boardPiece, Board boardToTest, vector<Move>* moves);
	void GetDiagonalMoveOptions(SDL_Point piecePosition, BoardPiece boardPiece, Board boardToTest, vector<Move>* moves);
	void GetKnightMoveOptions(SDL_Point piecePosition, BoardPiece boardPiece, Board boardToTest, vector<Move>* moves);
	void GetKingMoveOptions(SDL_Point piecePosition, BoardPiece boardPiece, Board boardToTest, vector<Move>* moves);

	void ClearEnPassant();
	bool CheckForCheck(Board boardToTest, COLOUR teamColour);
	bool CheckForCheckmate(Board boardToTest, COLOUR teamColour);
	bool CheckForStalemate(Board boardToCheck, COLOUR teamColour);

//--------------------------------------------------------------------------------------------------
protected:
	std::shared_ptr<SDL_Texture>		  mSelectAPieceTexture;
	SDL_Rect		  mPawnPromotionDrawPosition;

	Board*			  mChessBoard;
	COLOUR			  mTeamColour;
	vector<SDL_Point>* mHighlightPositions;		//Screen positioning, NOT board positioning.
	MOVETYPE		  mCurrentMove;
	SDL_Point*		  mSelectedPiecePosition = nullptr;

	int				  mNumberOfLivingPieces;

	bool			  mInCheck;

	Move*			  mLastMove;
};


#endif //_CHESSPLAYER_H
