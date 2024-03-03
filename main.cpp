#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <string>
#include <random>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/core/nvp.hpp>
#include <boost/container_hash/hash.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>


namespace mre{

struct content{
    friend class boost::serialization::access;
    using angle_type = std::size_t;

    inline content(angle_type angle): _angle(angle) {}
    inline angle_type angle() const { return _angle; }
    void reset_angle_random(){
        static std::random_device dev;
        static std::mt19937 rng_angle(dev());
        std::uniform_int_distribution<> angle_dist(0, 180);
        _angle = angle_dist(rng_angle);
    }
    void freeze(){
        // complicated deterministic business logic
        _angle = 0;
    }
    content frozen() const{
        mre::content copy(*this);
        copy.freeze();
        return copy;
    }

    static content generate(){
        static std::random_device  dev;
        static std::mt19937        rng(dev());
        std::uniform_real_distribution<> dist_length(-0.5f, 0.5f);

        mre::content content{0};
        content._length = dist_length(rng);
        content.reset_angle_random();
        return content;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        ar & boost::serialization::make_nvp("length",  _length);
        ar & boost::serialization::make_nvp("angle", _angle);
    }

    friend std::size_t hash_value(content const& c){
        std::size_t seed = 0;
        boost::hash_combine(seed, c._length);
        boost::hash_combine(seed, c._angle);
        return seed;
    }

    inline std::size_t hash() const { return boost::hash<mre::content>{}(*this); }
    inline std::size_t frozen_id() const { return frozen().hash(); }
    inline std::string id() const { return (boost::format("%1%~%2%-%3%") % frozen_id() % hash() % angle()).str(); }
    inline bool operator<(const content& other) const { return id() < other.id(); }
    public:
        double _length;
        angle_type _angle;

    private:
        content() = default;
};


struct parameters{
    std::size_t degree;
    std::size_t frame_size;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        ar & boost::serialization::make_nvp("degree",     degree);
        ar & boost::serialization::make_nvp("frame_size", frame_size);
    }
};

std::ostream& operator<<(std::ostream& stream, const mre::parameters& params);

struct package{
    friend class boost::serialization::access;

    struct tags{
        struct id{};
        struct content{};
        struct angle{};
        struct frozen{};
    };

    using container = boost::multi_index_container<
        mre::content,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<boost::multi_index::identity<mre::content>>,
            boost::multi_index::ordered_unique<boost::multi_index::tag<tags::id>, boost::multi_index::const_mem_fun<mre::content, std::string, &mre::content::id>>,
            boost::multi_index::ordered_non_unique<boost::multi_index::tag<tags::content>, boost::multi_index::const_mem_fun<mre::content, std::size_t, &mre::content::hash>>,
            boost::multi_index::ordered_non_unique<boost::multi_index::tag<tags::angle>, boost::multi_index::const_mem_fun<mre::content, mre::content::angle_type, &mre::content::angle>>,
            boost::multi_index::ordered_non_unique<boost::multi_index::tag<tags::frozen>, boost::multi_index::const_mem_fun<mre::content, std::size_t, &mre::content::frozen_id>>
        >
    >;

    inline explicit package(const mre::parameters& params): _loaded(false), _parameters(params) {}
    inline explicit package(): _loaded(false) {}
    void save(const std::string& filename) const;
    void load(const std::string& filename);
    inline std::size_t size() const { return _samples.size(); }
    inline bool loaded() const { return _loaded; }
    const mre::content& operator[](const std::string& id) const;
    const mre::parameters& params() const { return _parameters; }
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        ar & boost::serialization::make_nvp("samples", _samples);
        ar & boost::serialization::make_nvp("params",  _parameters);
    }

    auto begin() const { return _samples.begin(); }
    auto end() const { return _samples.end(); }

    public:
        std::size_t generate(std::size_t contents, std::size_t angles);
    private:
        bool _loaded;
        container  _samples;
        mre::parameters _parameters;
};

}

// { sources

std::ostream& mre::operator<<(std::ostream& stream, const mre::parameters& params){
    stream << "params {" << std::endl;
    stream << "    degree:     " << params.degree << std::endl;
    stream << "    frame_size: " << params.frame_size << std::endl;
    stream << "}";
    return stream;
}

void mre::package::save(const std::string& filename) const {
    std::ofstream stream(filename);
    try{
        boost::archive::text_oarchive out(stream, boost::archive::no_tracking);
        std::cout << "serialization library version: " << out.get_library_version() << std::endl;
        out << *this;
    } catch(const std::exception& e){
        std::cout << "Error saving archive: " << e.what() << std::endl;
    }
    stream.close();
}

void mre::package::load(const std::string& filename){
    std::ifstream stream(filename);
    try{
        boost::archive::text_iarchive in(stream, boost::archive::no_tracking);
        std::cout << "serialization library version: " << in.get_library_version() << std::endl;
        in >> *this;
        _loaded = true;
    } catch(const std::exception& e){
        std::cout << "Error loading archive: " << e.what() << std::endl;
    }
    stream.close();
}

std::size_t mre::package::generate(std::size_t contents, std::size_t angles){
    std::size_t count = 0;
    std::size_t v_content = 0;
    while(v_content++ < contents){
        mre::content x = mre::content::generate();
        std::size_t v_angle = 0;
        while(v_angle++ < angles){
            mre::content x_angle = x;
            x_angle.reset_angle_random(); // commenting out this line makes it work
            _samples.insert(x_angle);
            ++count;
        }
    }
    return count;
}

int main(int argc, char** argv){
    if(argc < 2){
        std::cout << "Usage: " << std::endl << argv[0] << " pack FILENAME N" << std::endl << argv[0] << " unpack FILENAME" <<std::endl;
        return 1;
    }
    if(argv[1] == std::string("pack")){
        auto params = mre::parameters{
            .degree     = 4,
            .frame_size = 128
        };
        mre::package package(params);
        std::size_t count = package.generate(boost::lexical_cast<std::size_t>(argv[3]), 4);
        std::size_t j = 0;
        for(auto i = package.begin(); i != package.end(); ++i){
            const mre::content& c = *i;
            std::cout << j++ << " " << c._length << " " << c.angle() << std::endl;
        }
        package.save(argv[2]);
        std::cout << "serialized: " << count << " contents" << std::endl;
        return 0;
    }else if(argv[1] == std::string("unpack")){
        mre::package package;
        package.load(argv[2]);
        if(package.loaded()){
            std::cout << "Package loaded: " << package.size() << std::endl << package.params() << std::endl;
            return 0;
        }
        return 1;
    }else{
        std::cout << "Usage: " << std::endl << argv[0] << " pack FILENAME N" << std::endl << argv[0] << " unpack FILENAME" <<std::endl;
        return 1;
    }

    return 0;
}
