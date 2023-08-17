#include "pch.h"
#include "Game.h"

void Game::loadMap()
{
	std::ifstream inputFile(this->mapFile);

	if (!inputFile.is_open())
	{
		std::cout << "Unable to open the file." << std::endl;
		throw std::runtime_error("Failed to load the map");
	}

	unsigned int lineCounter = 0; // Auxiliary variable for iterating through the vector
	std::string line;

	while (std::getline(inputFile, line))
	{
		line.pop_back();
		this->map.push_back({}); // Add an empty vector to our vector of vectors

		for (char c : line)
		{
			if (!std::isdigit(c)) // Check if the loaded value is a number; if not, an error occurs
			{
				throw std::runtime_error("Found an illegal character in the map file");
			}

			this->map[lineCounter].push_back(c - '0'); // Add the value to our empty vector
		}

		lineCounter++;
	}

	inputFile.close();
}

void Game::loadEntities()
{
	setPlayerChar(); // Set whether the game is for player 'P' or 'E'.

	std::ifstream inputFile(this->statusFile);

	if (!inputFile.is_open())
	{
		std::cout << "Unable to open the file." << std::endl;
		throw std::runtime_error("Failed to restore the status.txt file");
	}

	std::string goldLine; // Auxiliary variable for reading the amount of gold.
	long gold = 0; // Auxiliary variable for converting the string to a long.

	if (std::getline(inputFile, goldLine))
	{
		try
		{
			gold = std::stol(goldLine); // Attempt to convert.
		}
		catch (const std::invalid_argument& e)
		{
			throw std::runtime_error("Error, invalid argument. The value read is not a number");
		}
		catch (const std::out_of_range& e)
		{
			throw std::runtime_error("Error, value out of range for a long type");
		}

		playerBase.setGold(gold); // Set how much gold we have.
	}

	std::string line;
	int baseLine = 0; // Auxiliary variable that indicates if we've already read the base.

	while (std::getline(inputFile, line))
	{
		if (baseLine < 2) // Line where we load the base. There are always two bases, so < 2.
		{
			loadBase(line);
			baseLine++;
		}
		else // Lines where we load all other entities.
		{
			loadEntity(line);
		}
	}
	inputFile.close();
}

void Game::loadBase(const std::string baseParameters)
{
	// Below are auxiliary variables that we will read from the line.
	// We know that data from the file looks like this: P B 10 0 0 98 A
	// P or E in the first position - our base or enemy's base
	// B - object type 'base'
	// 10 - unique ID of the entity
	// 0 0 - position x y
	// 98 - health of the base
	// A - currently building unit 'A' (other options might be 'S', 'W', 'P', etc.)

	std::istringstream iss(baseParameters);
	char tmpPlayer = '-';
	char tmpBase = '-';
	int uniqueID = 0;
	int posX = 0;
	int posY = 0;
	int health = 0;
	char entityBuildType = '-';

	iss >> tmpPlayer >> tmpBase >> uniqueID >> posX >> posY >> health >> entityBuildType;

	setBaseParameters(tmpPlayer, uniqueID, posX, posY, entityBuildType);
	occupiedID[uniqueID] = uniqueID;
}

void Game::loadEntity(const std::string entityParameters)
{
	// Below are auxiliary variables that we will read from the line.
	// We know that data from the file looks like this: P K 13 20 10 5
	// P or E in the first position - our entity or enemy's entity
	// K - object type 'knight' (other options could be 'S', 'W', 'P', etc.)
	// 13 - unique ID of the entity
	// 20 10 - position x y
	// 5 - health of the entity

	std::istringstream iss(entityParameters);
	char tmpPlayer = '-';
	char entityType = '-';
	int uniqueID = 0;
	int posX = 0;
	int posY = 0;
	int health = 0;
	iss >> tmpPlayer >> entityType >> uniqueID >> posX >> posY >> health;

	if (entityParameters[0] == PlayerBaseChar) // Check if we're reading a line for our entity.
	{
		Entity* emptyEntity = createEntity(entityType);

		if (emptyEntity)
		{
			setEntityParameters(emptyEntity, uniqueID, posX, posY, health);
			PlayerEntities[emptyEntity->getEntityUniqueID()] = emptyEntity;
			occupiedID[uniqueID] = uniqueID;
		}
	}
	else // Load enemy entity
	{
		Entity* emptyEntity = createEntity(entityType);

		if (emptyEntity)
		{
			setEntityParameters(emptyEntity, uniqueID, posX, posY, health);
			EnemyEntities[emptyEntity->getEntityUniqueID()] = emptyEntity;
			occupiedID[uniqueID] = uniqueID;
		}
	}
}

Entity* Game::createEntity(char entityType)
{
	if (entityCreators.find(entityType) != entityCreators.end()) //Sprawdzenie czy mozemy stworzyc jednostke o danym typie.
	{
		return entityCreators[entityType](); //Tworzenie danej jednostki.
	}
	return nullptr;

}

void Game::setEntityParameters(Entity* emptyEntity,int ID, int posX, int posY, int health)
{
	emptyEntity->setEntityUniqueID(ID);
	emptyEntity->setPosition(posX, posY);
	emptyEntity->setHealth(health);
}

void Game::setBaseParameters(char Player, int ID, int posX, int posY, char entityBuildType)
{
	if (Player == PlayerBaseChar) // Set parameters for our base
	{
		playerBase.setBaseUniqueID(ID);
		playerBase.setPosition(posX, posY);
		playerBase.setBuildStatus(entityBuildType);
	}
	else if (Player == EnemyBaseChar) // Set parameters for the enemy base
	{
		enemyBase.setBaseUniqueID(ID);
		enemyBase.setPosition(posX, posY);
		enemyBase.setBuildStatus(entityBuildType);
	}
}

void Game::generateOrdersBase(char build_pay, char entityBuildType)
{
	std::ofstream ordersFile(this->ordersFile, std::ios::app);

	if (!ordersFile)
	{
		std::cerr << "Unable to open file" << '\n';
	}

	// Create a build or purchase order
	std::string newOrders = std::to_string(playerBase.getBaseUniqueID()) + " " + build_pay + " " + entityBuildType;

	ordersFile << newOrders << '\n'; // Write to file

	ordersFile.close();
}

void Game::generateOrdersMove(int entityID, char action, int posX, int posY)
{
	std::ofstream ordersFile(this->ordersFile, std::ios::app);

	if (!ordersFile)
	{
		std::cerr << "Unable to open file" << '\n';
	}

	// Create a movement order
	std::string newOrders = std::to_string(entityID) + " " + action + " " + std::to_string(posX) + " " + std::to_string(posY);

	ordersFile << newOrders << '\n'; // Write to file

	ordersFile.close();
}

void Game::generateOrdersAttack(int playerEntityID, int enemyEntityID)
{
	std::ofstream ordersFile(this->ordersFile, std::ios::app);

	if (!ordersFile)
	{
		std::cerr << "Unable to open file" << '\n';
	}

	// Create an attack order
	std::string newOrders = std::to_string(playerEntityID) + " " + 'A' + " " + std::to_string(enemyEntityID);

	ordersFile << newOrders << '\n'; // Write to file

	ordersFile.close();
}

void Game::setPlayerChar()
{
	if (playerNumberTurn == "1") // If we are player 1, our units will start with the letter 'P'.
	{
		PlayerBaseChar = 'P';
		EnemyBaseChar = 'E';
	}
	else if (playerNumberTurn == "2") // If we are player 2, our units will start with the letter 'E'.
	{
		PlayerBaseChar = 'E';
		EnemyBaseChar = 'P';
	}
}

void Game::buyEntity(char typeBuyedEntity)
{
	Entity* newEntity = createEntity(typeBuyedEntity); // Purchase a new unit
	setEntityParameters(newEntity, creatorID(), playerBase.getBasePosX(), playerBase.getBasePosY(), entityBirthHealth[typeBuyedEntity]); // Set the unit at the base
	PlayerEntities[creatorID()] = newEntity; // Add pointer to the hashing map
	playerBase.entityCostOfPurchase(entityCost[typeBuyedEntity]); // Unit purchase cost

	generateOrdersBase('P', newEntity->getClass()); // Create a purchase order
}

char Game::getBaseNumber()
{
	return this->PlayerBaseChar;
}

void Game::buildEntity(char typeBuildEntity)
{
	playerBase.setBuildStatus(typeBuildEntity); // Set status to "building unit"
	generateOrdersBase('B', typeBuildEntity); // Create a build order
}

int Game::creatorID()
{
	int newID = 0;

	if (!occupiedID.empty()) // Check if
	{
		newID = occupiedID.rbegin()->second + 1; // Create ID one greater in size than the previously built/purchased unit

		while (occupiedID.find(newID) != occupiedID.end()) // Check if a unit with such an ID exists
		{
			newID++; // If yes, increase the counter
		}

		occupiedID[newID] = newID; // If not, set such ID as occupied
	}

	return newID;
}

std::pair<int, int> Game::maxDistancePoint(int startX, int startY, int maxMoves)
{
	int rows = map.size();
	int cols = map[0].size();

	int dx[] = { -1, 1, 0, 0 }; // Vertical shifts
	int dy[] = { 0, 0, -1, 1 }; // Horizontal shifts

	std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));
	std::queue<std::pair<int, int>> q;

	q.push({ startX, startY });
	visited[startX][startY] = true;

	std::pair<int, int> currentPoint = { startX, startY };

	while (!q.empty() && maxMoves >= 0)
	{
		int levelSize = q.size();

		for (int i = 0; i < levelSize; ++i)
		{
			int x = q.front().first;
			int y = q.front().second;
			q.pop();

			currentPoint = { x, y };

			if (x == enemyBase.getBasePosX() && y == enemyBase.getBasePosY())
			{
				return currentPoint; // If we've reached the target location, return this point
			}

			for (int dir = 0; dir < 4; ++dir)
			{
				int nx = x + dx[dir];
				int ny = y + dy[dir];

				if (nx >= 0 && nx < rows && ny >= 0 && ny < cols && map[nx][ny] != 9 && !visited[nx][ny])
				{
					q.push({ nx, ny });
					visited[nx][ny] = true;
				}
			}
		}

		--maxMoves; // Decrease the number of moves
	}

	return currentPoint;
}

void Game::buildRandomEntity()
{
	if (playerBase.isReadyToBuild())
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int> distribution(0, 6);

		int random_number = distribution(gen);

		playerBase.setBuildStatus(entityTypesForBuildOrBuy[random_number]); // Set our base to build the selected unit type

		generateOrdersBase('B', entityTypesForBuildOrBuy[random_number]); // Generate a build order
	}
}

void Game::buyRandomEntity()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> distribution(0, 6);

	int random_number = distribution(gen);

	int counter = 0;
	while (counter < 7) // Loop through all available units for purchase
	{
		if (playerBase.isReadyToBuy(entityCost[entityTypesForBuildOrBuy[random_number]])) // Check if we can afford to buy the chosen unit
		{
			playerBase.entityCostOfPurchase(entityCost[entityTypesForBuildOrBuy[random_number]]); // Buy the unit if possible
			generateOrdersBase('P', entityTypesForBuildOrBuy[random_number]); // Generate a purchase order
			break;
		}
		else
		{
			// If not, try to buy other units
			random_number++;

			if (random_number == 7) // If this is the last unit (numbers chosen in order as in the task file)
			{
				random_number = 0; // Start from the beginning of the queue
			}
		}

		counter++;
	}
}

int Game::distaneBetweenBaseAndAttacker(Entity* attacker)
{
	return std::abs(attacker->getPosX() - enemyBase.getBasePosX()) + std::abs(attacker->getPosY() - enemyBase.getBasePosY());
}

void Game::moveWithAllEntity()
{
	for (const auto& unit : PlayerEntities)
	{
		// Calculate the farthest position we can reach
		std::pair<int, int> newPosition;
		newPosition = maxDistancePoint(unit.second->getPosX(), unit.second->getPosY(), unit.second->getMovementSpeed());
		if (newPosition.first != enemyBase.getBasePosX() && newPosition.second != enemyBase.getBasePosY())
		{
			unit.second->setPosition(newPosition.first, newPosition.second);
			generateOrdersMove(unit.second->getEntityUniqueID(), 'M', newPosition.first, newPosition.second); // Generate a move order
		}
	}
}

void Game::attackWithAllEntity()
{
	for (const auto& unit : PlayerEntities)
	{
		// Calculate the distance between us and the target unit
		int distanceFromEnemyBase;
		distanceFromEnemyBase = distaneBetweenBaseAndAttacker(unit.second);
		if (distanceFromEnemyBase <= unit.second->getAttackRange()) // If the distance is less than our attack range, attack
		{
			enemyBase.takeDamage(damageFromEntity[unit.second->getClass()]);
			generateOrdersAttack(unit.second->getEntityUniqueID(), enemyBase.getBaseUniqueID()); // Generate an attack order
		}
	}
}

void Game::mainAlgorithm()
{
	// Call all functions sequentially, main loop of the program
	loadMap();
	loadEntities();
	buildRandomEntity();
	buyRandomEntity();
	moveWithAllEntity();
	attackWithAllEntity();
}




