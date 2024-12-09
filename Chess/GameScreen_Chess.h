//------------------------------------------------------------------------
//  Author: Paul Roberts 2016
//------------------------------------------------------------------------

#pragma once

#include "ChessCommons.h"
#include "ChessPlayer.h"
#include <vector>

class Texture2D;

class GameScreen_Chess
{
//--------------------------------------------------------------------------------------------------
public:
	GameScreen_Chess(sdl_game::app_context & context);
	~GameScreen_Chess();

	void Render(sdl_game::app_context & context);
	void Update(float deltaTime, SDL_Event e);

//--------------------------------------------------------------------------------------------------
private:
	void RenderBoard(sdl_game::app_context & context);
	void RenderPiece(sdl_game::app_context & context, BoardPiece boardPiece, SDL_Point position);
	void RenderHighlights(sdl_game::app_context & context);
	void RenderPreTurnText(sdl_game::app_context & context);

//--------------------------------------------------------------------------------------------------
private:
	std::shared_ptr<SDL_Texture>		 mBackgroundTexture;
	std::shared_ptr<SDL_Texture>		 mWhitePiecesSpritesheet;
	std::shared_ptr<SDL_Texture>		 mBlackPiecesSpritesheet;
	std::shared_ptr<SDL_Texture>		 mSelectedPieceSpritesheet;
	std::shared_ptr<SDL_Texture>		 mSquareHighlightSpritesheet;
	std::shared_ptr<SDL_Texture>		 mPreviousMoveHighlightSpritesheet;
	std::shared_ptr<SDL_Texture>		 mGameStateSpritesheet;
	Board*			 mChessBoard;

	ChessPlayer*	 mPlayers[2];
	COLOUR			 mPlayerTurn;
	bool			 mAIPlayerPlaying;

	SDL_Point		 mSelectedPiecePosition;
	vector<SDL_Point> mHighlightPositions;
	bool			 mHighlightsOn;
	Move*			 mLastMove;

	TURNSTATE		 mTurnState;
	GAMESTATE		 mGameState;
	float			 mPreTurnTextTimer;

	int*			 mSearchDepth;
};
