#include "Precompiled.hpp"

#include "ReflectionParser.hpp"

#include "LanguageTypes/Class.hpp"
#include "LanguageTypes/Global.hpp"
#include "LanguageTypes/Function.hpp"
#include "LanguageTypes/Enum.hpp"

#include <iostream>
#include <fstream>

#include <boost/algorithm/string.hpp>

#define RECURSE_NAMESPACES(kind, cursor, method, ns, cd) \
    if (kind == CXCursor_Namespace)                  \
    {                                                \
        auto displayName = cursor.GetDisplayName(); \
        if (!displayName.empty())                   \
        {                                            \
            ns.emplace_back(displayName);          \
            method(cursor, ns, cd);                    \
            ns.pop_back();                          \
        }                                            \
    }                                                \

ReflectionParser::ReflectionParser(const ReflectionOptions &options)
    : m_options(options)
    , m_index(nullptr)
    , m_translationUnit(nullptr)
{
   
}

ReflectionParser::~ReflectionParser(void)
{
    for (auto *klass : m_classes)
        delete klass;

    for (auto *global : m_globals)
        delete global;

    for (auto *globalFunction : m_globalFunctions)
        delete globalFunction;

    for (auto *enewm : m_enums)
        delete enewm;

    if (m_translationUnit)
        clang_disposeTranslationUnit(m_translationUnit);

    if (m_index)
        clang_disposeIndex(m_index);
}

void ReflectionParser::CollectFiles(std::string const & inPath, ReflectionParser::tStringList & outFiles)
{
	boost::filesystem::recursive_directory_iterator itEnd, it(inPath);
	while (it != itEnd)
	{
		if (!boost::filesystem::is_directory(*it))
		{
			boost::filesystem::path path = *it;
			std::string filename = path.string();
			std::string ext = GetFileExtension(filename);
			std::transform(ext.begin(), ext.end(), ext.begin(), tolower);

			if ((ext == "hpp") || (ext == "h"))
			{
				outFiles.push_back(filename);
			}
		}

		try
		{
			++it;
		}
		catch (std::exception & ex)
		{
			std::cout << ex.what() << std::endl;
			it.no_push();
			try
			{
				++it;
			}
			catch (...)
			{
				std::cout << ex.what() << std::endl;
				return;
			}
		}
	}
}

void ReflectionParser::Parse(void)
{
	tStringList filesList;
	CollectFiles(m_options.inputPath, filesList);

	// ensure that output directory exist
	fs::create_directory(fs::path(m_options.outputPath));

	for (tStringList::const_iterator it = filesList.begin(); it != filesList.end(); ++it)
	{
		std::cout << "Parse header " << *it << "..." << std::endl;
		ProcessFile(*it);
	}
}

//CXChildVisitResult visitCursor(CXCursor cursor, CXCursor parent, CXClientData client_data)
//{
//    ReflectionParser * parser = (ReflectionParser*)client_data;
//    assert(parser != 0);
//    MacrosManager & manager = parser->GetMacrosManager();
//
//    Cursor c(cursor);
//  
//    if (c.GetKind() == CXCursor_MacroExpansion)
//    {
//        std::string const name = c.GetDisplayName();
//        if (MacrosInfo::RequestProcess(name))
//        {
//            CXFile file;
//            unsigned int line, column, offset;
//            CXSourceLocation loc = clang_getCursorLocation(cursor);
//            clang_getSpellingLocation(loc, &file, &line, &column, &offset);
//
//            std::string fileName = clang_getCString(clang_getFileName(file));
//
//            if (parser->IsCurrentFile(fileName))
//            {
//                CXSourceRange range = clang_getCursorExtent(cursor);
//                std::string code = parser->GetCodeInRange(range.begin_int_data, range.end_int_data);
//                
//                MacrosInfo info(name, code, line, column, fileName);
//                manager.AddMacros(info);
//            }            
//        }
//    }
//    
//    return CXChildVisit_Continue;
//}


void ReflectionParser::ProcessFile(std::string const & fileName)
{   
	m_index = clang_createIndex(true, false);
    
	std::vector<const char *> arguments;

	for (auto &argument : m_options.arguments)
	{
		// unescape flags
		boost::algorithm::replace_all(argument, "\\-", "-");

		arguments.emplace_back(argument.c_str());
	}

	m_translationUnit = clang_createTranslationUnitFromSourceFile(
		m_index,
		fileName.c_str(),
		static_cast<int>(arguments.size()),
		arguments.data(),
		0,
		nullptr
		);

	auto cursor = clang_getTranslationUnitCursor(m_translationUnit);

    std::string fileId = GetFileID(fileName);
	Namespace tempNamespace;
    std::stringstream outCode;

    // includes
    outCode << "#include \"wrap/sc_memory.hpp\"\n\n\n";

	buildClasses(cursor, tempNamespace, outCode);
	tempNamespace.clear();
    for (auto it = m_classes.begin(); it != m_classes.end(); ++it)
    {
        Class const * klass = *it;
        if (klass->ShouldGenerate())
        {
            outCode << "#define " << fileId << "_body ";
            klass->GenerateCode(outCode);
        }
    }

	/*buildGlobals(cursor, tempNamespace);
	tempNamespace.clear();

	buildGlobalFunctions(cursor, tempNamespace);
	tempNamespace.clear();

	buildEnums(cursor, tempNamespace);*/

    /// write ScFileID definition
    outCode << "\n\n#undef ScFileID\n";
    outCode << "#define ScFileID " << fileId;

    
    /// test dump
    //outCode << "\n\n ----- Dump ----- \n\n";
    //DumpTree(cursor, 0, outCode);

	// generate output file
	fs::path outputPath(m_options.outputPath);
	outputPath /= fs::path(GetOutputFileName(fileName));
	std::ofstream outputFile(outputPath.string());
    outputFile << outCode.str();
	outputFile.close();
}


void ReflectionParser::buildClasses(const Cursor &cursor, Namespace &currentNamespace, std::stringstream & outCode)
{
    for (auto &child : cursor.GetChildren())
    {
        auto kind = child.GetKind();

        // actual definition and a class or struct
        if (child.IsDefinition() && (kind == CXCursor_ClassDecl || kind == CXCursor_StructDecl))
        {
            m_classes.emplace_back(new Class(child, currentNamespace));
        }
        
        RECURSE_NAMESPACES(kind, child, buildClasses, currentNamespace, outCode);
    }
}


void ReflectionParser::DumpTree(Cursor const & cursor, int level, std::stringstream & outData)
{
    outData << "\n";
    for (size_t i = 0; i < level; ++i)
        outData << "-";

    outData << cursor.GetDisplayName() << ", " << cursor.GetKind();

    for (auto &child : cursor.GetChildren())
    {
        DumpTree(child, level + 1, outData);
    }
}

//void ReflectionParser::buildGlobals(const Cursor &cursor, Namespace &currentNamespace)
//{
//    for (auto &child : cursor.GetChildren())
//    {
//        // skip static globals (hidden)
//        if (child.GetStorageClass() == CX_SC_Static)
//            continue;
//
//        auto kind = child.GetKind();
//
//        // variable declaration, which is global
//        if (kind == CXCursor_VarDecl) 
//        {
//            m_globals.emplace_back(
//                new Global(child, currentNamespace) 
//           );
//        }
//
//        RECURSE_NAMESPACES(kind, child, buildGlobals, currentNamespace);
//    }
//}
//
//void ReflectionParser::buildGlobalFunctions(const Cursor &cursor, Namespace &currentNamespace)
//{
//    for (auto &child : cursor.GetChildren())
//    {
//        // skip static globals (hidden)
//        if (child.GetStorageClass() == CX_SC_Static)
//            continue;
//
//        auto kind = child.GetKind();
//
//        // function declaration, which is global
//        if (kind == CXCursor_FunctionDecl) 
//        {
//            m_globalFunctions.emplace_back(new Function(child, currentNamespace));
//        }
//
//        RECURSE_NAMESPACES(
//            kind, 
//            child, 
//            buildGlobalFunctions, 
//            currentNamespace 
//       );
//    }
//}
//
//void ReflectionParser::buildEnums(const Cursor &cursor, Namespace &currentNamespace)
//{
//    for (auto &child : cursor.GetChildren())
//    {
//        auto kind = child.GetKind();
//
//        // actual definition and an enum
//        if (child.IsDefinition() && kind == CXCursor_EnumDecl)
//        {
//            // anonymous enum if the underlying type display name contains this
//            if (child.GetType().GetDisplayName().find("anonymous enum at") 
//                != std::string::npos)
//            {
//                // anonymous enums are just loaded as 
//                // globals with each of their values
//                Enum::LoadAnonymous(m_globals, child, currentNamespace);
//            }
//            else
//            {
//                m_enums.emplace_back(
//                    new Enum(child, currentNamespace) 
//               );
//            }
//        }
//
//        RECURSE_NAMESPACES(kind, child, buildEnums, currentNamespace);
//    }
//}

std::string ReflectionParser::GetFileExtension(std::string const & fileName)
{
	// get file extension
	size_t n = fileName.rfind(".");
	if (n == std::string::npos)
		return std::string();

	return fileName.substr(n + 1, std::string::npos);
}

std::string ReflectionParser::GetOutputFileName(std::string const & fileName)
{
	fs::path inputPath(fileName);
	std::string baseName = inputPath.filename().replace_extension().string();
	return baseName + ".generated" + inputPath.extension().string();
}

std::string ReflectionParser::GetFileID(std::string const & fileName)
{
    fs::path inputPath(fileName);
    std::string res = inputPath.filename().string();
    
    for (std::string::iterator it = res.begin(); it != res.end(); ++it)
    {
        if (*it == ' ' || *it == '-' || *it == '.')
        {
            *it = '_';
        }
    }

    return res;
}
