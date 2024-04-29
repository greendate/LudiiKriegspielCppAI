#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <stdexcept>
#include <vector>
#include <cstring>
#include "ludii_cpp_ai_LudiiKriegspielCppAI.h"

// NOTE: String descriptions of signatures of Java methods can be found by
// navigating to the directory containing the .class files and using:
//
// javap -s <ClassName>.class

// Java classes we'll need
jclass clsLudiiGameWrapper;
jclass clsLudiiStateWrapper;

// Java method IDs for all the methods we'll need to call
jmethodID midLudiiGameWrapperCtor;
jmethodID midIsStochasticGame;
jmethodID midIsImperfectInformationGame;
jmethodID midIsSimultaneousMoveGame;
jmethodID midNumPlayers;

jmethodID midLudiiStateWrapperCtor;
jmethodID midLudiiStateWrapperCopyCtor;
jmethodID midIsTerminal;
jmethodID midLegalMovesArray;
jmethodID midApplyMove;
jmethodID midCurrentPlayer;
jmethodID midRunRandomPlayout;
jmethodID midReturns;
int32_t player = -1;
int32_t opponent = -1;
int32_t players = 2;

/**
 * This code is a short example demonstrating the usage of Kriegspiel-specific Ludii functions
 * For more information regarding Ludii AI development, please refer to the following: 
	* https://www.ludii.games/index.php
	* https://ludiitutorials.readthedocs.io/en/latest/basic_ai_api.html
	* https://ludiitutorials.readthedocs.io/en/latest/ludii_terminology.html
	* https://ludiitutorials.readthedocs.io/en/latest/cheat_sheet.html
	* https://github.com/Ludeme/Ludii_AI_Cpp
 
 @author Nikola Novarlic
*/

/**
	* It is good practice to call this after basically any call to a Java method
	* that might throw an exception.
*/
static void CheckJniException(JNIEnv* jenv) {
	if (jenv->ExceptionCheck()) {
		jenv->ExceptionDescribe();
		jenv->ExceptionClear();
		printf("Java Exception at line %d of %s\n", __LINE__, __FILE__);
		throw std::runtime_error("Java exception thrown!");
	}
}

/**
	* Perform some static initialisation by caching all the Java classes and
	* Java Method IDs we need.
*/
JNIEXPORT void JNICALL Java_ludii_1cpp_1ai_LudiiKriegspielCppAI_nativeStaticInit
(JNIEnv* jenv, jclass)
{
	// Classes
	clsLudiiGameWrapper = (jclass) jenv->NewGlobalRef(jenv->FindClass("utils/LudiiGameWrapper"));
	CheckJniException(jenv);

	clsLudiiStateWrapper = (jclass) jenv->NewGlobalRef(jenv->FindClass("utils/LudiiStateWrapper"));
	CheckJniException(jenv);

	// Game wrapper methods
	midLudiiGameWrapperCtor = jenv->GetMethodID(clsLudiiGameWrapper, "<init>", "(Lgame/Game;)V");
	CheckJniException(jenv);

	midIsStochasticGame = jenv->GetMethodID(clsLudiiGameWrapper, "isStochasticGame", "()Z");
	CheckJniException(jenv);

	midIsImperfectInformationGame = jenv->GetMethodID(clsLudiiGameWrapper, "isImperfectInformationGame", "()Z");
	CheckJniException(jenv);

	midIsSimultaneousMoveGame = jenv->GetMethodID(clsLudiiGameWrapper, "isSimultaneousMoveGame", "()Z");
	CheckJniException(jenv);

	midNumPlayers = jenv->GetMethodID(clsLudiiGameWrapper, "numPlayers", "()I");
	CheckJniException(jenv);

	// State wrapper methods
	midLudiiStateWrapperCtor = jenv->GetMethodID(clsLudiiStateWrapper, "<init>", "(Lutils/LudiiGameWrapper;Lother/context/Context;)V");
	CheckJniException(jenv);

	midLudiiStateWrapperCopyCtor = jenv->GetMethodID(clsLudiiStateWrapper, "<init>", "(Lutils/LudiiStateWrapper;)V");
	CheckJniException(jenv);

	midIsTerminal = jenv->GetMethodID(clsLudiiStateWrapper, "isTerminal", "()Z");
	CheckJniException(jenv);

	midLegalMovesArray = jenv->GetMethodID(clsLudiiStateWrapper, "legalMovesArray", "()[Lother/move/Move;");
	CheckJniException(jenv);

	midApplyMove = jenv->GetMethodID(clsLudiiStateWrapper, "applyMove", "(Lother/move/Move;)V");
	CheckJniException(jenv);

	midCurrentPlayer = jenv->GetMethodID(clsLudiiStateWrapper, "currentPlayer", "()I");
	CheckJniException(jenv);

	midRunRandomPlayout = jenv->GetMethodID(clsLudiiStateWrapper, "runRandomPlayout", "()V");
	CheckJniException(jenv);

	midReturns = jenv->GetMethodID(clsLudiiStateWrapper, "returns", "()[D");
	CheckJniException(jenv);
}

/**
 	* The function returns True if the Agent plays with white pieces
*/
static bool aiPlayerWhite() {
	return (player == 1);
}

static jobject chooseRandomMove(JNIEnv* jenv, std::mt19937 rng, jobjectArray pseudoLegalMoves, jsize numPseudoMoves) {
	std::uniform_int_distribution<> distr(0, numPseudoMoves - 1);
    int32_t r = distr(rng);
	return jenv->GetObjectArrayElement(pseudoLegalMoves, r);
}

static int getMoveFrom(JNIEnv* jenv, jclass moveObjectClass, jobject moveObject) {
	jfieldID from = jenv->GetFieldID(moveObjectClass, "from", "I");
	CheckJniException(jenv);
	return jenv->GetIntField(moveObject, from);
}

static int getMoveTo(JNIEnv* jenv, jclass moveObjectClass, jobject moveObject) {
	jfieldID to = jenv->GetFieldID(moveObjectClass, "to", "I");
	CheckJniException(jenv);
	return jenv->GetIntField(moveObject, to);
}

static int getMoveWhat(JNIEnv* jenv, jclass moveObjectClass, jobject moveObject) {
	jmethodID what = jenv->GetMethodID(moveObjectClass, "what", "()I");
	CheckJniException(jenv);
	return jenv->CallIntMethod(moveObject, what);
}

static const char* getMoveActionDescription(JNIEnv* jenv, jclass moveObjectClass, jobject moveObject) {
	jmethodID actionDescriptionStringShort = jenv->GetMethodID(moveObjectClass, "actionDescriptionStringShort", "()Ljava/lang/String;");
	CheckJniException(jenv);
	jstring actionDescriptionJstring = (jstring) jenv->CallObjectMethod(moveObject, actionDescriptionStringShort);
	CheckJniException(jenv);
	const char *actionDescription = jenv->GetStringUTFChars(actionDescriptionJstring, 0);
	return actionDescription;
}

JNIEXPORT jobject JNICALL Java_ludii_1cpp_1ai_LudiiKriegspielCppAI_nativeSelectAction
(JNIEnv* jenv, jobject jobjAI, jobject game, jobject context, jdouble maxSeconds,
		jint maxIterations, jint maxDepth, jstring lastTryMessages, jstring refereeMessages)
{
	std::random_device dev;
	std::mt19937 rng(dev());

	jobject wrappedGame = jenv->NewObject(clsLudiiGameWrapper, midLudiiGameWrapperCtor, game);
	CheckJniException(jenv);
	jobject wrappedRootState = jenv->NewObject(clsLudiiStateWrapper, midLudiiStateWrapperCtor, wrappedGame, context);
	CheckJniException(jenv);

	/**
		* A function that returns a set of pseudo-legal moves
		* As a player, the agent has to try a move selected among the given set
		* The referee, who knows the list of legal moves for both sides, answers whether the move was legal or not
	*/
    const jobjectArray pseudoLegalMoves =
		      static_cast<jobjectArray>(jenv->CallObjectMethod(wrappedRootState, midLegalMovesArray));
    CheckJniException(jenv);
	const jsize numPseudoMoves = jenv->GetArrayLength(pseudoLegalMoves);

	/**
		* Messages received from the referee after the last try
		* This string contains messages separated by a semicolon
		* Messages announce the position of all captures, checks, and check directions,
		* or simply an empty string if the move is legal without checks and captures.
		* Otherwise, if the selected try is impossible to play, the only message announced is "Illegal move"  
	*/
	const char *lastTryMessagesCString = jenv->GetStringUTFChars(lastTryMessages, 0);

	if (strcmp(lastTryMessagesCString, "Illegal move") == 0) {
		/** 
			* The previously selected try is illegal on the referee's board
			* The Agent is asked for another try
			* For the simplicity terms, we would again try a random move
		*/ 
		return chooseRandomMove(jenv, rng, pseudoLegalMoves, numPseudoMoves);
	}

	jobject firstMoveRef = jenv->GetObjectArrayElement(pseudoLegalMoves, 0);
	jclass moveCls = jenv->GetObjectClass(firstMoveRef);
	const char *moveActionDescription = getMoveActionDescription(jenv, moveCls, firstMoveRef);

	if(strcmp(moveActionDescription, "Promote") == 0) {
		/**
			* A situation in which the agent's pawn needs to be promoted after the last move
			* In this case, pseudoLegalMoves would contain only promotion moves with different pieces
			* Use getMoveWhat() to select a move containing the desired promotion piece
			* We will play a random promotion move
		*/
		int32_t firstPromotionPiece = getMoveWhat(jenv, moveCls, firstMoveRef);
		CheckJniException(jenv);
		return chooseRandomMove(jenv, rng, pseudoLegalMoves, numPseudoMoves);
	}

	/**
	 	* The approach to iterate through all pseudo-legal moves
	*/
    for (jsize i = 0; i < numPseudoMoves; ++i) {
        jobject moveLocalRef = jenv->GetObjectArrayElement(pseudoLegalMoves, i);
		int32_t moveFrom = getMoveFrom(jenv, moveCls, moveLocalRef);
		CheckJniException(jenv);
		int32_t moveTo = getMoveTo(jenv, moveCls, moveLocalRef);
		CheckJniException(jenv);
    }
    
	jclass contextCls = jenv->GetObjectClass(context);
	CheckJniException(jenv);

	jmethodID midPlayerTries = jenv->GetMethodID(contextCls, "score", "(I)I");
	CheckJniException(jenv);

	/** 
		* Opponent tries represents the number of pawn captures 
		* available to the opponent after our last legal move
	*/
	int32_t opponentTries = (int32_t) jenv->CallIntMethod(context, midPlayerTries, (jint) opponent);
	CheckJniException(jenv);

	/**
		* Referee messages that are announced after the opponent's legal move
		* If the capture happened in the last turn, one of the messages would 
		* specify where the capture took place and whether the captured piece is the pawn or another piece
		* If the agent's King is in check, one message would reveal the check type (rank, file, long/short diagonal, or knight) 
	*/
	const char *refereeMessagesCString = jenv->GetStringUTFChars(refereeMessages, 0);

	/**
		* Pawn tries denotes the number of legal capturing moves using pawns 
		* for the current turn
	*/
	int32_t pawnTries = (int32_t) jenv->CallIntMethod(context, midPlayerTries, (jint) player);
	CheckJniException(jenv);

	/**
		* After receiving all the available information, the agent is asked to provide the next try
		* The next try needs to be selected among the set of pseudo-legal moves based on the agent's strategy
	*/
	return chooseRandomMove(jenv, rng, pseudoLegalMoves, numPseudoMoves);
}

JNIEXPORT void JNICALL Java_ludii_1cpp_1ai_LudiiKriegspielCppAI_nativeInitAI
(JNIEnv* jenv, jobject jobjAI, jobject game, jint playerID)
{
	player = (int32_t) playerID;
    opponent = (int32_t) players - player + 1;
}

JNIEXPORT void JNICALL Java_ludii_1cpp_1ai_LudiiKriegspielCppAI_nativeCloseAI
(JNIEnv* jenv, jobject jobjAI)
{
	// Nothing to do here

	// NOTE: could consider deleting the global refs for Java classes here,
	// but then we'd also have to create them again in nativeInitAI(), instead
	// of only in the nativeStaticInit() function
}

JNIEXPORT jboolean JNICALL Java_ludii_1cpp_1ai_LudiiKriegspielCppAI_nativeSupportsGame
(JNIEnv* jenv, jobject jobjAI, jobject game)
{
	return true;
}

