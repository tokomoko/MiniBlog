# MiniBlog

**MiniBlog** is a sample blog application built with [MiniSpring](https://github.com/tokomoko/MiniSpring), a Spring-inspired backend framework written from scratch in C++17.

The project was created primarily to test MiniSpring and demonstrate how its different components work together in a complete application, from HTTP request handling and routing to database operations, object mapping, and authentication.

MiniBlog includes a simple web interface, a REST-style API, SQLite persistence, Markdown rendering, and administrator authentication.

## Features

- **HTTP Routing** — Endpoint registration, HTTP methods, and URL path parameters.
- **Controllers and Services** — Separation of HTTP request handling and application logic.
- **Reflection and Metadata** — Custom C++ type descriptors and field metadata used for object mapping and ORM operations.
- **ORM and Repositories** — Entity management, database queries, and object persistence.
- **SQLite** — Database integration with schema migrations.
- **JSON Mapping** — Serialization of C++ objects into JSON and deserialization of incoming JSON requests.
- **Authentication and Authorization** — User authentication, password encoding, and role-based endpoint protection.
- **Static Files** — Serving the application's frontend resources.
- **Markdown** — Rendering blog post content as HTML.

## Architecture

MiniBlog follows a controller-service-repository structure built on top of MiniSpring.

A simplified request flow looks like this:

```text
                Browser
                   |
                   v
              HTTP Server
                   |
                   v
                 Router
                   |
                   v
               Controller
                   |
                   v
                Service
                   |
                   v
               Repository
                   |
                   v
              EntityManager
                   |
                   v
              SQLite Database
```

Controllers handle HTTP requests and responses, services contain application logic, and repositories provide access to persisted entities through MiniSpring's ORM.

MiniSpring handles the underlying HTTP infrastructure, routing, entity management, JSON mapping, and security integration.

## Application Initialization

The application is initialized using MiniSpring's modular architecture.

Database, HTTP, and security modules are registered and configured independently.

```cpp
MiniSpringApp app;

// Configure database
auto& database = app.modules().add<DatabaseModule>();
database.configuration()
    .file("blog.db")
    .migrations("resources/migrations");

// Configure HTTP server
auto& http = app.modules().add<HttpModule>();
http.configuration()
    .port(8080)
    .threads(4)
    .keepAlive(true);

// Configure static file serving
http.staticFiles().directory("resources/static");

// Configure security
auto& security = app.modules().add<SecurityModule>();
security.configuration()
    .cookieName("SESSIONID")
    .frameOptions(FrameOptions::DENY)
    .referrerPolicy(
        ReferrerPolicy::STRICT_ORIGIN_WHEN_CROSS_ORIGIN
    );
```

After module configuration, application types and services are registered.

```cpp
// Register application types
app.services().get<EntityRegistry>()
    .registerEntity<Post>()
    .registerEntity<PostCreateRequest>()
    .registerEntity<PostUpdateRequest>()
    .registerEntity<User>()
    .registerEntity<LoginRequest>();

// Register application services
app.services().add<MarkdownRenderer>(
    std::make_unique<MarkdownRenderer>()
);

app.services().add<IPasswordEncoder>(
    std::make_unique<PBKDF2PasswordEncoder>()
);

app.services().add<IUserDetailsService>(
    std::make_unique<AppUserDetailsService>(app.services())
);

// Configure and build the application
app.configure();
app.build();

// Apply database migrations and seed administrator data
database.migrate();
AdminSeeder::seed(app.services());

// Register repositories and services
auto repository = std::make_unique<PostRepository>(
    app.services().get<EntityManager>()
);

auto service = std::make_unique<PostService>(*repository);

app.services().add<PostRepository>(std::move(repository));
app.services().add<PostService>(std::move(service));

// Register controllers
http.controllers().registerController<PostController>(app);
http.controllers().registerController<AuthController>(app);

// Start HTTP server
app.listen();
```

This example demonstrates how MiniSpring modules, services, entity registration, repositories, and controllers are assembled into an application.

## Controllers and HTTP Routing

MiniBlog uses controller classes to organize HTTP endpoints.

For example, `PostController` registers routes for retrieving, creating, updating, and deleting blog posts.

```cpp
class PostController : public Controller {
public:
    PostController(MiniSpringApp& app)
        : m_service(app.services().get<PostService>()),
          m_mapper(app.services().get<JsonMapper>()),
          m_markdownRenderer(
              app.services().get<MarkdownRenderer>()
          ),
          m_security(app.modules().get<SecurityModule>()) {}

    void registerRoutes(Router& router) override {

        // Retrieve all posts
        router.addRoute("GET", "/api/posts",
            [this](HttpRequest& req) {
                HttpResponse res;

                auto posts = m_service.getAllPosts();
                res.json(m_mapper.toJson(posts));

                return Result<HttpResponse, HttpError>::success(res);
            });

        // Retrieve a post by ID
        router.addRoute("GET", "/api/posts/{id}",
            [this](HttpRequest& req) {
                HttpResponse res;

                std::string id = req.pathParameter("id");
                auto post = m_service.getPost(id);

                if (!post)
                    return Result<HttpResponse, HttpError>::error(
                        HttpError(404, "Post not found")
                    );

                auto json = m_mapper.toJson(post.value());
                json["html"] =
                    m_markdownRenderer.render(post->content);

                res.json(json);

                return Result<HttpResponse, HttpError>::success(res);
            });

        // Create a post (ADMIN authorization required)
        router.addRoute("POST", "/api/posts",
            [this](HttpRequest& req) {
                HttpResponse res;

                PostCreateRequest request =
                    req.json<PostCreateRequest>();

                if (request.title.empty())
                    return Result<HttpResponse, HttpError>::error(
                        HttpError(400, "Title is required")
                    );

                Post post = m_service.create(request);
                res.json(m_mapper.toJson(post));

                return Result<HttpResponse, HttpError>::success(res);
            },
            m_security.protect({"ADMIN"})
        );

        // Update a post (ADMIN authorization required)
        router.addRoute("PUT", "/api/posts/{id}",
            [this](HttpRequest& req) {
                HttpResponse res;

                std::string id = req.pathParameter("id");
                PostUpdateRequest update =
                    req.json<PostUpdateRequest>();

                auto post = m_service.update(id, update);
                res.json(m_mapper.toJson(post));

                return Result<HttpResponse, HttpError>::success(res);
            },
            m_security.protect({"ADMIN"})
        );

        // Delete a post (ADMIN authorization required)
        router.addRoute("DELETE", "/api/posts/{id}",
            [this](HttpRequest& req) {
                HttpResponse res;

                std::string id = req.pathParameter("id");
                m_service.deletePost(id);

                res.status(204);

                return Result<HttpResponse, HttpError>::success(res);
            },
            m_security.protect({"ADMIN"})
        );
    }

private:
    PostService& m_service;
    JsonMapper& m_mapper;
    MarkdownRenderer& m_markdownRenderer;
    SecurityModule& m_security;
};
```

The framework provides the routing and request/response abstractions, while the controller connects HTTP requests to the application's services.

### Available API endpoints

| Method | Endpoint | Description | Access |
|---|---|---|---|
| GET | `/api/posts` | Retrieve all posts | Public |
| GET | `/api/posts/{id}` | Retrieve a post | Public |
| POST | `/api/posts` | Create a post | ADMIN |
| PUT | `/api/posts/{id}` | Update a post | ADMIN |
| DELETE | `/api/posts/{id}` | Delete a post | ADMIN |

## Reflection and Entity Mapping

MiniSpring uses a custom reflection and metadata system to describe C++ types and their fields.

Since C++17 does not provide built-in runtime reflection, entity metadata is explicitly defined using type descriptors and field information.

For example, the `Post` model describes its database representation as follows:

```cpp
class Post {
public:
    std::string id;
    std::string title;
    std::string content;
    int createdAt = 0;

    static EntityDescriptor<Post> descriptor() {
        TypeDescriptor<Post> type;

        type.addField(
            std::make_shared<FieldInfo<Post, std::string>>(
                "id",
                &Post::id,
                FieldMetadata(FieldFlag::PrimaryKey)
            )
        );

        type.addField(
            std::make_shared<FieldInfo<Post, std::string>>(
                "title",
                &Post::title,
                FieldMetadata(FieldFlag::None)
            )
        );

        type.addField(
            std::make_shared<FieldInfo<Post, std::string>>(
                "content",
                &Post::content,
                FieldMetadata(FieldFlag::None)
            )
        );

        type.addField(
            std::make_shared<FieldInfo<Post, int>>(
                "createdAt",
                &Post::createdAt,
                FieldMetadata(FieldFlag::None)
            )
        );

        return EntityDescriptor<Post>("posts", type);
    }
};
```

The descriptor associates the C++ type with its database table and describes its fields, types, and metadata.

This information is used by MiniSpring's entity management and ORM infrastructure for object mapping and database operations.

## Repository Layer

MiniBlog uses repositories to separate database access from application logic.

A repository can extend MiniSpring's generic `Repository<T>` functionality with application-specific methods.

```cpp
class PostRepository : public Repository<Post> {
public:
    using Repository<Post>::Repository;

    std::vector<Post> findNewest() {
        auto posts = findAll();

        std::sort(
            posts.begin(),
            posts.end(),
            [](auto& a, auto& b) {
                return a.createdAt > b.createdAt;
            }
        );

        return posts;
    }

    std::optional<Post> findBySlug(std::string slug) {
        return findById(Value(slug));
    }
};
```

The repository uses the inherited ORM operations and adds methods specific to blog posts.

## Authentication and Authorization

MiniBlog integrates with MiniSpring's security module through application-specific implementations of its user-details interfaces.

For example, `AppUserDetails` adapts the application's `User` model to MiniSpring's `UserDetails` abstraction:

```cpp
class AppUserDetails : public UserDetails {
public:
    explicit AppUserDetails(User user)
        : m_user(std::move(user)) {}

    const std::string& username() const override {
        return m_user.username;
    }

    const std::string& passwordHash() const override {
        return m_user.passwordHash;
    }

    std::vector<std::string> authorities() const override {
        return m_user.admin
            ? std::vector<std::string>{"ADMIN"}
            : std::vector<std::string>{"USER"};
    }

private:
    User m_user;
};
```

The corresponding `AppUserDetailsService` retrieves users through the `EntityManager`:

```cpp
class AppUserDetailsService : public IUserDetailsService {
public:
    explicit AppUserDetailsService(ServiceProvider& services)
        : m_services(services) {}

    std::optional<std::shared_ptr<UserDetails>>
    loadUserByUsername(const std::string& username) override {

        auto& manager = m_services.get<EntityManager>();

        auto user = manager.findBy<User>(
            "username",
            Value(username)
        );

        if (!user)
            return std::nullopt;

        return std::shared_ptr<UserDetails>(
            std::make_shared<AppUserDetails>(*user)
        );
    }

private:
    ServiceProvider& m_services;
};
```

This allows MiniSpring's security infrastructure to retrieve application users and their authorities without depending directly on MiniBlog's user model.

## Getting Started

### Clone the repository

MiniSpring is included as a Git submodule under `vendor/`. Clone recursively to initialize MiniSpring and its own dependencies.

```bash
git clone --recursive https://github.com/tokomoko/MiniBlog.git
```

If you have already cloned the repository without its submodules:

```bash
git submodule update --init --recursive
```

### Build

Open the project in Visual Studio and build using the included project configuration.

The application uses SQLite for persistent storage and creates or updates its database schema through MiniSpring's migration system.

## Project Purpose

MiniBlog is primarily a demonstration and testing application for MiniSpring.

It was built to exercise the framework's components together in a practical application and to explore how its abstractions can be used to develop a backend without relying on an existing web framework.

The project is not intended to be a standalone blogging platform. Its main purpose is to demonstrate MiniSpring's capabilities through a working application.

## Technologies

- **C++17**
- **MiniSpring** — Backend framework
- **SQLite** — Database
- **HTML / CSS / JavaScript** — Frontend
- **Markdown** — Post content rendering