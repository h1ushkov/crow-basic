#include <crow.h>
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

int main() {
    crow::SimpleApp app;

    // Define a connection string to PostgreSQL
    pqxx::connection conn("user=postgres dbname=iboard password=123456 hostaddr=127.0.0.1 port=5432");

    // Define a route to handle POST requests
    CROW_ROUTE(app, "/post")
    .methods(crow::HTTPMethod::Post)
    ([&conn](const crow::request& req) {
        try {
            // Parse JSON from the request body
            json requestJson = json::parse(req.body);

            // Extract information from JSON
            string title = requestJson["title"];
            string content = requestJson["content"];

            // Create a new database table if not exists
            pqxx::work txn(conn);
            txn.exec("CREATE TABLE IF NOT EXISTS posts (id SERIAL PRIMARY KEY, title VARCHAR, content VARCHAR)");
            txn.commit();

            // Insert data into the table
            pqxx::work txn2(conn);
            txn2.exec_params("INSERT INTO posts (title, content) VALUES ($1, $2)", title, content);
            txn2.commit();

            // Return a response
            return crow::response{ "Post added successfully!" };
        } catch (const std::exception& e) {
            return crow::response(500, e.what());
        }
    });

    // Define a route to handle GET requests for posts
    CROW_ROUTE(app, "/posts")
    .methods(crow::HTTPMethod::Get)
    ([&conn]() {
        try {
            // Fetch posts from the database
            pqxx::nontransaction txn(conn);
            pqxx::result result = txn.exec("SELECT * FROM posts");

            // Convert result to JSON
            json posts;
            for (const auto& row : result) {
                json post;
                post["id"] = row["id"].as<int>();
                post["title"] = row["title"].as<string>();
                post["content"] = row["content"].as<string>();
                posts.push_back(post);
            }

            // Return JSON response
            return crow::response{ posts.dump() };
        } catch (const std::exception& e) {
            return crow::response(500, e.what());
        }
    });

    // Start the Crow server
    app.port(8080).multithreaded().run();

    return 0;
}
