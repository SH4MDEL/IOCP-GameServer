#pragma once
#include "scene.h"
#include "object.h"
#include "piece.h"

class MainScene : public Scene
{
public:
	MainScene();
	virtual ~MainScene();

	void Update(float timeElapsed) final;
	void Render(const shared_ptr<sf::RenderWindow>& window) final;
	
	void OnProcessingKeyboardMessage(sf::Event inputEvent) final;
	void OnProcessingMouseMessage(sf::Event inputEvent, const shared_ptr<sf::RenderWindow>& window) final;

	void ProcessPacket(char* buf) final;

	void AddPlayer(int id, sf::Vector2f position, const char* name);
	void ExitPlayer(int id);

	void Move(INT id, sf::Vector2f position);
	void SetChat(INT id, const char* chat);

private:
	virtual void BuildObjects() final;
	virtual void DestroyObject() final;

private:
	shared_ptr<sf::Texture>	m_boardTexture;
	shared_ptr<sf::Texture>	m_pieceTexture;

	shared_ptr<Object> m_whiteTile;
	shared_ptr<Object> m_blackTile;

	shared_ptr<Piece> m_avatar;
	unordered_map<INT, shared_ptr<Piece>> m_players;
};