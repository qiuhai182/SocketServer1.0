#pragma once

#include <map>
#include <string>
#include <mutex>


// 根据后缀格式“.xx”获取对应文本类型


const std::pair<std::string, std::string> pairArray[] =
{
    std::make_pair(".load", "text/html"),
    std::make_pair(".123", "application/vnd.lotus-1-2-3"),
    std::make_pair(".3ds", "image/x-3ds"),
    std::make_pair(".3g2", "video/3gpp"),
    std::make_pair(".3ga", "video/3gpp"),
    std::make_pair(".3gp", "video/3gpp"),
    std::make_pair(".3gpp", "video/3gpp"),
    std::make_pair(".602", "application/x-t602"),
    std::make_pair(".669", "audio/x-mod"),
    std::make_pair(".7z", "application/x-7z-compressed"),
    std::make_pair(".a", "application/x-archive"),
    std::make_pair(".aac", "audio/mp4"),
    std::make_pair(".abw", "application/x-abiword"),
    std::make_pair(".abw.crashed", "application/x-abiword"),
    std::make_pair(".abw.gz", "application/x-abiword"),
    std::make_pair(".ac3", "audio/ac3"),
    std::make_pair(".ace", "application/x-ace"),
    std::make_pair(".adb", "text/x-adasrc"),
    std::make_pair(".ads", "text/x-adasrc"),
    std::make_pair(".afm", "application/x-font-afm"),
    std::make_pair(".ag", "image/x-applix-graphics"),
    std::make_pair(".ai", "application/illustrator"),
    std::make_pair(".aif", "audio/x-aiff"),
    std::make_pair(".aifc", "audio/x-aiff"),
    std::make_pair(".aiff", "audio/x-aiff"),
    std::make_pair(".al", "application/x-perl"),
    std::make_pair(".alz", "application/x-alz"),
    std::make_pair(".amr", "audio/amr"),
    std::make_pair(".ani", "application/x-navi-animation"),
    std::make_pair(".anim[1-9j]", "video/x-anim"),
    std::make_pair(".anx", "application/annodex"),
    std::make_pair(".ape", "audio/x-ape"),
    std::make_pair(".arj", "application/x-arj"),
    std::make_pair(".arw", "image/x-sony-arw"),
    std::make_pair(".as", "application/x-applix-spreadsheet"),
    std::make_pair(".asc", "text/plain"),
    std::make_pair(".asf", "video/x-ms-asf"),
    std::make_pair(".asp", "application/x-asp"),
    std::make_pair(".ass", "text/x-ssa"),
    std::make_pair(".asx", "audio/x-ms-asx"),
    std::make_pair(".atom", "application/atom+xml"),
    std::make_pair(".au", "audio/basic"),
    std::make_pair(".avi", "video/x-msvideo"),
    std::make_pair(".aw", "application/x-applix-word"),
    std::make_pair(".awb", "audio/amr-wb"),
    std::make_pair(".awk", "application/x-awk"),
    std::make_pair(".axa", "audio/annodex"),
    std::make_pair(".axv", "video/annodex"),
    std::make_pair(".bak", "application/x-trash"),
    std::make_pair(".bcpio", "application/x-bcpio"),
    std::make_pair(".bdf", "application/x-font-bdf"),
    std::make_pair(".bib", "text/x-bibtex"),
    std::make_pair(".bin", "application/octet-stream"),
    std::make_pair(".blend", "application/x-blender"),
    std::make_pair(".blender", "application/x-blender"),
    std::make_pair(".bmp", "image/bmp"),
    std::make_pair(".bz", "application/x-bzip"),
    std::make_pair(".bz2", "application/x-bzip"),
    std::make_pair(".c", "text/x-csrc"),
    std::make_pair(".c++", "text/x-c++src"),
    std::make_pair(".cab", "application/vnd.ms-cab-compressed"),
    std::make_pair(".cb7", "application/x-cb7"),
    std::make_pair(".cbr", "application/x-cbr"),
    std::make_pair(".cbt", "application/x-cbt"),
    std::make_pair(".cbz", "application/x-cbz"),
    std::make_pair(".cc", "text/x-c++src"),
    std::make_pair(".cdf", "application/x-netcdf"),
    std::make_pair(".cdr", "application/vnd.corel-draw"),
    std::make_pair(".cer", "application/x-x509-ca-cert"),
    std::make_pair(".cert", "application/x-x509-ca-cert"),
    std::make_pair(".cgm", "image/cgm"),
    std::make_pair(".chm", "application/x-chm"),
    std::make_pair(".chrt", "application/x-kchart"),
    std::make_pair(".class", "application/x-java"),
    std::make_pair(".cls", "text/x-tex"),
    std::make_pair(".cmake", "text/x-cmake"),
    std::make_pair(".cpio", "application/x-cpio"),
    std::make_pair(".cpio.gz", "application/x-cpio-compressed"),
    std::make_pair(".cpp", "text/x-c++src"),
    std::make_pair(".cr2", "image/x-canon-cr2"),
    std::make_pair(".crt", "application/x-x509-ca-cert"),
    std::make_pair(".crw", "image/x-canon-crw"),
    std::make_pair(".cs", "text/x-csharp"),
    std::make_pair(".csh", "application/x-csh"),
    std::make_pair(".css", "text/css"),
    std::make_pair(".cssl", "text/css"),
    std::make_pair(".csv", "text/csv"),
    std::make_pair(".cue", "application/x-cue"),
    std::make_pair(".cur", "image/x-win-bitmap"),
    std::make_pair(".cxx", "text/x-c++src"),
    std::make_pair(".d", "text/x-dsrc"),
    std::make_pair(".dar", "application/x-dar"),
    std::make_pair(".dbf", "application/x-dbf"),
    std::make_pair(".dc", "application/x-dc-rom"),
    std::make_pair(".dcl", "text/x-dcl"),
    std::make_pair(".dcm", "application/dicom"),
    std::make_pair(".dcr", "image/x-kodak-dcr"),
    std::make_pair(".dds", "image/x-dds"),
    std::make_pair(".deb", "application/x-deb"),
    std::make_pair(".der", "application/x-x509-ca-cert"),
    std::make_pair(".desktop", "application/x-desktop"),
    std::make_pair(".dia", "application/x-dia-diagram"),
    std::make_pair(".diff", "text/x-patch"),
    std::make_pair(".divx", "video/x-msvideo"),
    std::make_pair(".djv", "image/vnd.djvu"),
    std::make_pair(".djvu", "image/vnd.djvu"),
    std::make_pair(".dng", "image/x-adobe-dng"),
    std::make_pair(".doc", "application/msword"),
    std::make_pair(".docbook", "application/docbook+xml"),
    std::make_pair(".docm", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"),
    std::make_pair(".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"),
    std::make_pair(".dot", "text/vnd.graphviz"),
    std::make_pair(".dsl", "text/x-dsl"),
    std::make_pair(".dtd", "application/xml-dtd"),
    std::make_pair(".dtx", "text/x-tex"),
    std::make_pair(".dv", "video/dv"),
    std::make_pair(".dvi", "application/x-dvi"),
    std::make_pair(".dvi.bz2", "application/x-bzdvi"),
    std::make_pair(".dvi.gz", "application/x-gzdvi"),
    std::make_pair(".dwg", "image/vnd.dwg"),
    std::make_pair(".dxf", "image/vnd.dxf"),
    std::make_pair(".e", "text/x-eiffel"),
    std::make_pair(".egon", "application/x-egon"),
    std::make_pair(".eif", "text/x-eiffel"),
    std::make_pair(".el", "text/x-emacs-lisp"),
    std::make_pair(".emf", "image/x-emf"),
    std::make_pair(".emp", "application/vnd.emusic-emusic_package"),
    std::make_pair(".ent", "application/xml-external-parsed-entity"),
    std::make_pair(".eps", "image/x-eps"),
    std::make_pair(".eps.bz2", "image/x-bzeps"),
    std::make_pair(".eps.gz", "image/x-gzeps"),
    std::make_pair(".epsf", "image/x-eps"),
    std::make_pair(".epsf.bz2", "image/x-bzeps"),
    std::make_pair(".epsf.gz", "image/x-gzeps"),
    std::make_pair(".epsi", "image/x-eps"),
    std::make_pair(".epsi.bz2", "image/x-bzeps"),
    std::make_pair(".epsi.gz", "image/x-gzeps"),
    std::make_pair(".epub", "application/epub+zip"),
    std::make_pair(".erl", "text/x-erlang"),
    std::make_pair(".es", "application/ecmascript"),
    std::make_pair(".etheme", "application/x-e-theme"),
    std::make_pair(".etx", "text/x-setext"),
    std::make_pair(".exe", "application/x-ms-dos-executable"),
    std::make_pair(".exr", "image/x-exr"),
    std::make_pair(".ez", "application/andrew-inset"),
    std::make_pair(".f", "text/x-fortran"),
    std::make_pair(".f90", "text/x-fortran"),
    std::make_pair(".f95", "text/x-fortran"),
    std::make_pair(".fb2", "application/x-fictionbook+xml"),
    std::make_pair(".fig", "image/x-xfig"),
    std::make_pair(".fits", "image/fits"),
    std::make_pair(".fl", "application/x-fluid"),
    std::make_pair(".flac", "audio/x-flac"),
    std::make_pair(".flc", "video/x-flic"),
    std::make_pair(".fli", "video/x-flic"),
    std::make_pair(".flv", "video/x-flv"),
    std::make_pair(".flw", "application/x-kivio"),
    std::make_pair(".fo", "text/x-xslfo"),
    std::make_pair(".for", "text/x-fortran"),
    std::make_pair(".g3", "image/fax-g3"),
    std::make_pair(".gb", "application/x-gameboy-rom"),
    std::make_pair(".gba", "application/x-gba-rom"),
    std::make_pair(".gcrd", "text/directory"),
    std::make_pair(".ged", "application/x-gedcom"),
    std::make_pair(".gedcom", "application/x-gedcom"),
    std::make_pair(".gen", "application/x-genesis-rom"),
    std::make_pair(".gf", "application/x-tex-gf"),
    std::make_pair(".gg", "application/x-sms-rom"),
    std::make_pair(".gif", "image/gif"),
    std::make_pair(".glade", "application/x-glade"),
    std::make_pair(".gmo", "application/x-gettext-translation"),
    std::make_pair(".gnc", "application/x-gnucash"),
    std::make_pair(".gnd", "application/gnunet-directory"),
    std::make_pair(".gnucash", "application/x-gnucash"),
    std::make_pair(".gnumeric", "application/x-gnumeric"),
    std::make_pair(".gnuplot", "application/x-gnuplot"),
    std::make_pair(".gp", "application/x-gnuplot"),
    std::make_pair(".gpg", "application/pgp-encrypted"),
    std::make_pair(".gplt", "application/x-gnuplot"),
    std::make_pair(".gra", "application/x-graphite"),
    std::make_pair(".gsf", "application/x-font-type1"),
    std::make_pair(".gsm", "audio/x-gsm"),
    std::make_pair(".gtar", "application/x-tar"),
    std::make_pair(".gv", "text/vnd.graphviz"),
    std::make_pair(".gvp", "text/x-google-video-pointer"),
    std::make_pair(".gz", "application/x-gzip"),
    std::make_pair(".h", "text/x-chdr"),
    std::make_pair(".h++", "text/x-c++hdr"),
    std::make_pair(".hdf", "application/x-hdf"),
    std::make_pair(".hh", "text/x-c++hdr"),
    std::make_pair(".hp", "text/x-c++hdr"),
    std::make_pair(".hpgl", "application/vnd.hp-hpgl"),
    std::make_pair(".hpp", "text/x-c++hdr"),
    std::make_pair(".hs", "text/x-haskell"),
    std::make_pair(".htm", "text/html"),
    std::make_pair(".html", "text/html"),
    std::make_pair(".hwp", "application/x-hwp"),
    std::make_pair(".hwt", "application/x-hwt"),
    std::make_pair(".hxx", "text/x-c++hdr"),
    std::make_pair(".ica", "application/x-ica"),
    std::make_pair(".icb", "image/x-tga"),
    std::make_pair(".icns", "image/x-icns"),
    std::make_pair(".ico", "image/vnd.microsoft.icon"),
    std::make_pair(".ics", "text/calendar"),
    std::make_pair(".idl", "text/x-idl"),
    std::make_pair(".ief", "image/ief"),
    std::make_pair(".iff", "image/x-iff"),
    std::make_pair(".ilbm", "image/x-ilbm"),
    std::make_pair(".ime", "text/x-imelody"),
    std::make_pair(".imy", "text/x-imelody"),
    std::make_pair(".ins", "text/x-tex"),
    std::make_pair(".iptables", "text/x-iptables"),
    std::make_pair(".iso", "application/x-cd-image"),
    std::make_pair(".iso9660", "application/x-cd-image"),
    std::make_pair(".it", "audio/x-it"),
    std::make_pair(".j2k", "image/jp2"),
    std::make_pair(".jad", "text/vnd.sun.j2me.app-descriptor"),
    std::make_pair(".jar", "application/x-java-archive"),
    std::make_pair(".java", "text/x-java"),
    std::make_pair(".jng", "image/x-jng"),
    std::make_pair(".jnlp", "application/x-java-jnlp-file"),
    std::make_pair(".jp2", "image/jp2"),
    std::make_pair(".jpc", "image/jp2"),
    std::make_pair(".jpe", "image/jpeg"),
    std::make_pair(".jpeg", "image/jpeg"),
    std::make_pair(".jpf", "image/jp2"),
    std::make_pair(".jpg", "image/jpeg"),
    std::make_pair(".jpr", "application/x-jbuilder-project"),
    std::make_pair(".jpx", "image/jp2"),
    std::make_pair(".js", "application/javascript"),
    std::make_pair(".json", "application/json"),
    std::make_pair(".jsonp", "application/jsonp"),
    std::make_pair(".k25", "image/x-kodak-k25"),
    std::make_pair(".kar", "audio/midi"),
    std::make_pair(".karbon", "application/x-karbon"),
    std::make_pair(".kdc", "image/x-kodak-kdc"),
    std::make_pair(".kdelnk", "application/x-desktop"),
    std::make_pair(".kexi", "application/x-kexiproject-sqlite3"),
    std::make_pair(".kexic", "application/x-kexi-connectiondata"),
    std::make_pair(".kexis", "application/x-kexiproject-shortcut"),
    std::make_pair(".kfo", "application/x-kformula"),
    std::make_pair(".kil", "application/x-killustrator"),
    std::make_pair(".kino", "application/smil"),
    std::make_pair(".kml", "application/vnd.google-earth.kml+xml"),
    std::make_pair(".kmz", "application/vnd.google-earth.kmz"),
    std::make_pair(".kon", "application/x-kontour"),
    std::make_pair(".kpm", "application/x-kpovmodeler"),
    std::make_pair(".kpr", "application/x-kpresenter"),
    std::make_pair(".kpt", "application/x-kpresenter"),
    std::make_pair(".kra", "application/x-krita"),
    std::make_pair(".ksp", "application/x-kspread"),
    std::make_pair(".kud", "application/x-kugar"),
    std::make_pair(".kwd", "application/x-kword"),
    std::make_pair(".kwt", "application/x-kword"),
    std::make_pair(".la", "application/x-shared-library-la"),
    std::make_pair(".latex", "text/x-tex"),
    std::make_pair(".ldif", "text/x-ldif"),
    std::make_pair(".lha", "application/x-lha"),
    std::make_pair(".lhs", "text/x-literate-haskell"),
    std::make_pair(".lhz", "application/x-lhz"),
    std::make_pair(".log", "text/x-log"),
    std::make_pair(".ltx", "text/x-tex"),
    std::make_pair(".lua", "text/x-lua"),
    std::make_pair(".lwo", "image/x-lwo"),
    std::make_pair(".lwob", "image/x-lwo"),
    std::make_pair(".lws", "image/x-lws"),
    std::make_pair(".ly", "text/x-lilypond"),
    std::make_pair(".lyx", "application/x-lyx"),
    std::make_pair(".lz", "application/x-lzip"),
    std::make_pair(".lzh", "application/x-lha"),
    std::make_pair(".lzma", "application/x-lzma"),
    std::make_pair(".lzo", "application/x-lzop"),
    std::make_pair(".m", "text/x-matlab"),
    std::make_pair(".m15", "audio/x-mod"),
    std::make_pair(".m2t", "video/mpeg"),
    std::make_pair(".m3u", "audio/x-mpegurl"),
    std::make_pair(".m3u8", "audio/x-mpegurl"),
    std::make_pair(".m4", "application/x-m4"),
    std::make_pair(".m4a", "audio/mp4"),
    std::make_pair(".m4b", "audio/x-m4b"),
    std::make_pair(".m4v", "video/mp4"),
    std::make_pair(".mab", "application/x-markaby"),
    std::make_pair(".man", "application/x-troff-man"),
    std::make_pair(".mbox", "application/mbox"),
    std::make_pair(".md", "application/x-genesis-rom"),
    std::make_pair(".mdb", "application/vnd.ms-access"),
    std::make_pair(".mdi", "image/vnd.ms-modi"),
    std::make_pair(".me", "text/x-troff-me"),
    std::make_pair(".med", "audio/x-mod"),
    std::make_pair(".metalink", "application/metalink+xml"),
    std::make_pair(".mgp", "application/x-magicpoint"),
    std::make_pair(".mid", "audio/midi"),
    std::make_pair(".midi", "audio/midi"),
    std::make_pair(".mif", "application/x-mif"),
    std::make_pair(".minipsf", "audio/x-minipsf"),
    std::make_pair(".mka", "audio/x-matroska"),
    std::make_pair(".mkv", "video/x-matroska"),
    std::make_pair(".ml", "text/x-ocaml"),
    std::make_pair(".mli", "text/x-ocaml"),
    std::make_pair(".mm", "text/x-troff-mm"),
    std::make_pair(".mmf", "application/x-smaf"),
    std::make_pair(".mml", "text/mathml"),
    std::make_pair(".mng", "video/x-mng"),
    std::make_pair(".mo", "application/x-gettext-translation"),
    std::make_pair(".mo3", "audio/x-mo3"),
    std::make_pair(".moc", "text/x-moc"),
    std::make_pair(".mod", "audio/x-mod"),
    std::make_pair(".mof", "text/x-mof"),
    std::make_pair(".moov", "video/quicktime"),
    std::make_pair(".mov", "video/quicktime"),
    std::make_pair(".movie", "video/x-sgi-movie"),
    std::make_pair(".mp+", "audio/x-musepack"),
    std::make_pair(".mp2", "video/mpeg"),
    std::make_pair(".mp3", "audio/mpeg"),
    std::make_pair(".mp4", "video/mp4"),
    std::make_pair(".mpc", "audio/x-musepack"),
    std::make_pair(".mpe", "video/mpeg"),
    std::make_pair(".mpeg", "video/mpeg"),
    std::make_pair(".mpg", "video/mpeg"),
    std::make_pair(".mpga", "audio/mpeg"),
    std::make_pair(".mpp", "audio/x-musepack"),
    std::make_pair(".mrl", "text/x-mrml"),
    std::make_pair(".mrml", "text/x-mrml"),
    std::make_pair(".mrw", "image/x-minolta-mrw"),
    std::make_pair(".ms", "text/x-troff-ms"),
    std::make_pair(".msi", "application/x-msi"),
    std::make_pair(".msod", "image/x-msod"),
    std::make_pair(".msx", "application/x-msx-rom"),
    std::make_pair(".mtm", "audio/x-mod"),
    std::make_pair(".mup", "text/x-mup"),
    std::make_pair(".mxf", "application/mxf"),
    std::make_pair(".n64", "application/x-n64-rom"),
    std::make_pair(".nb", "application/mathematica"),
    std::make_pair(".nc", "application/x-netcdf"),
    std::make_pair(".nds", "application/x-nintendo-ds-rom"),
    std::make_pair(".nef", "image/x-nikon-nef"),
    std::make_pair(".nes", "application/x-nes-rom"),
    std::make_pair(".nfo", "text/x-nfo"),
    std::make_pair(".not", "text/x-mup"),
    std::make_pair(".nsc", "application/x-netshow-channel"),
    std::make_pair(".nsv", "video/x-nsv"),
    std::make_pair(".o", "application/x-object"),
    std::make_pair(".obj", "application/x-tgif"),
    std::make_pair(".ocl", "text/x-ocl"),
    std::make_pair(".oda", "application/oda"),
    std::make_pair(".odb", "application/vnd.oasis.opendocument.database"),
    std::make_pair(".odc", "application/vnd.oasis.opendocument.chart"),
    std::make_pair(".odf", "application/vnd.oasis.opendocument.formula"),
    std::make_pair(".odg", "application/vnd.oasis.opendocument.graphics"),
    std::make_pair(".odi", "application/vnd.oasis.opendocument.image"),
    std::make_pair(".odm", "application/vnd.oasis.opendocument.text-master"),
    std::make_pair(".odp", "application/vnd.oasis.opendocument.presentation"),
    std::make_pair(".ods", "application/vnd.oasis.opendocument.spreadsheet"),
    std::make_pair(".odt", "application/vnd.oasis.opendocument.text"),
    std::make_pair(".oga", "audio/ogg"),
    std::make_pair(".ogg", "video/x-theora+ogg"),
    std::make_pair(".ogm", "video/x-ogm+ogg"),
    std::make_pair(".ogv", "video/ogg"),
    std::make_pair(".ogx", "application/ogg"),
    std::make_pair(".old", "application/x-trash"),
    std::make_pair(".oleo", "application/x-oleo"),
    std::make_pair(".opml", "text/x-opml+xml"),
    std::make_pair(".ora", "image/openraster"),
    std::make_pair(".orf", "image/x-olympus-orf"),
    std::make_pair(".otc", "application/vnd.oasis.opendocument.chart-template"),
    std::make_pair(".otf", "application/x-font-otf"),
    std::make_pair(".otg", "application/vnd.oasis.opendocument.graphics-template"),
    std::make_pair(".oth", "application/vnd.oasis.opendocument.text-web"),
    std::make_pair(".otp", "application/vnd.oasis.opendocument.presentation-template"),
    std::make_pair(".ots", "application/vnd.oasis.opendocument.spreadsheet-template"),
    std::make_pair(".ott", "application/vnd.oasis.opendocument.text-template"),
    std::make_pair(".owl", "application/rdf+xml"),
    std::make_pair(".oxt", "application/vnd.openofficeorg.extension"),
    std::make_pair(".p", "text/x-pascal"),
    std::make_pair(".p10", "application/pkcs10"),
    std::make_pair(".p12", "application/x-pkcs12"),
    std::make_pair(".p7b", "application/x-pkcs7-certificates"),
    std::make_pair(".p7s", "application/pkcs7-signature"),
    std::make_pair(".pack", "application/x-java-pack200"),
    std::make_pair(".pak", "application/x-pak"),
    std::make_pair(".par2", "application/x-par2"),
    std::make_pair(".pas", "text/x-pascal"),
    std::make_pair(".patch", "text/x-patch"),
    std::make_pair(".pbm", "image/x-portable-bitmap"),
    std::make_pair(".pcd", "image/x-photo-cd"),
    std::make_pair(".pcf", "application/x-cisco-vpn-settings"),
    std::make_pair(".pcf.gz", "application/x-font-pcf"),
    std::make_pair(".pcf.z", "application/x-font-pcf"),
    std::make_pair(".pcl", "application/vnd.hp-pcl"),
    std::make_pair(".pcx", "image/x-pcx"),
    std::make_pair(".pdb", "chemical/x-pdb"),
    std::make_pair(".pdc", "application/x-aportisdoc"),
    std::make_pair(".pdf", "application/pdf"),
    std::make_pair(".pdf.bz2", "application/x-bzpdf"),
    std::make_pair(".pdf.gz", "application/x-gzpdf"),
    std::make_pair(".pef", "image/x-pentax-pef"),
    std::make_pair(".pem", "application/x-x509-ca-cert"),
    std::make_pair(".perl", "application/x-perl"),
    std::make_pair(".pfa", "application/x-font-type1"),
    std::make_pair(".pfb", "application/x-font-type1"),
    std::make_pair(".pfx", "application/x-pkcs12"),
    std::make_pair(".pgm", "image/x-portable-graymap"),
    std::make_pair(".pgn", "application/x-chess-pgn"),
    std::make_pair(".pgp", "application/pgp-encrypted"),
    std::make_pair(".php", "application/x-php"),
    std::make_pair(".php3", "application/x-php"),
    std::make_pair(".php4", "application/x-php"),
    std::make_pair(".pict", "image/x-pict"),
    std::make_pair(".pict1", "image/x-pict"),
    std::make_pair(".pict2", "image/x-pict"),
    std::make_pair(".pickle", "application/python-pickle"),
    std::make_pair(".pk", "application/x-tex-pk"),
    std::make_pair(".pkipath", "application/pkix-pkipath"),
    std::make_pair(".pkr", "application/pgp-keys"),
    std::make_pair(".pl", "application/x-perl"),
    std::make_pair(".pla", "audio/x-iriver-pla"),
    std::make_pair(".pln", "application/x-planperfect"),
    std::make_pair(".pls", "audio/x-scpls"),
    std::make_pair(".pm", "application/x-perl"),
    std::make_pair(".png", "image/png"),
    std::make_pair(".pnm", "image/x-portable-anymap"),
    std::make_pair(".pntg", "image/x-macpaint"),
    std::make_pair(".po", "text/x-gettext-translation"),
    std::make_pair(".por", "application/x-spss-por"),
    std::make_pair(".pot", "text/x-gettext-translation-template"),
    std::make_pair(".ppm", "image/x-portable-pixmap"),
    std::make_pair(".pps", "application/vnd.ms-powerpoint"),
    std::make_pair(".ppt", "application/vnd.ms-powerpoint"),
    std::make_pair(".pptm", "application/vnd.openxmlformats-officedocument.presentationml.presentation"),
    std::make_pair(".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"),
    std::make_pair(".ppz", "application/vnd.ms-powerpoint"),
    std::make_pair(".prc", "application/x-palm-database"),
    std::make_pair(".ps", "application/postscript"),
    std::make_pair(".ps.bz2", "application/x-bzpostscript"),
    std::make_pair(".ps.gz", "application/x-gzpostscript"),
    std::make_pair(".psd", "image/vnd.adobe.photoshop"),
    std::make_pair(".psf", "audio/x-psf"),
    std::make_pair(".psf.gz", "application/x-gz-font-linux-psf"),
    std::make_pair(".psflib", "audio/x-psflib"),
    std::make_pair(".psid", "audio/prs.sid"),
    std::make_pair(".psw", "application/x-pocket-word"),
    std::make_pair(".pw", "application/x-pw"),
    std::make_pair(".py", "text/x-python"),
    std::make_pair(".pyc", "application/x-python-bytecode"),
    std::make_pair(".pyo", "application/x-python-bytecode"),
    std::make_pair(".qif", "image/x-quicktime"),
    std::make_pair(".qt", "video/quicktime"),
    std::make_pair(".qtif", "image/x-quicktime"),
    std::make_pair(".qtl", "application/x-quicktime-media-link"),
    std::make_pair(".qtvr", "video/quicktime"),
    std::make_pair(".ra", "audio/vnd.rn-realaudio"),
    std::make_pair(".raf", "image/x-fuji-raf"),
    std::make_pair(".ram", "application/ram"),
    std::make_pair(".rar", "application/x-rar"),
    std::make_pair(".ras", "image/x-cmu-raster"),
    std::make_pair(".raw", "image/x-panasonic-raw"),
    std::make_pair(".rax", "audio/vnd.rn-realaudio"),
    std::make_pair(".rb", "application/x-ruby"),
    std::make_pair(".rdf", "application/rdf+xml"),
    std::make_pair(".rdfs", "application/rdf+xml"),
    std::make_pair(".reg", "text/x-ms-regedit"),
    std::make_pair(".rej", "application/x-reject"),
    std::make_pair(".rgb", "image/x-rgb"),
    std::make_pair(".rle", "image/rle"),
    std::make_pair(".rm", "application/vnd.rn-realmedia"),
    std::make_pair(".rmj", "application/vnd.rn-realmedia"),
    std::make_pair(".rmm", "application/vnd.rn-realmedia"),
    std::make_pair(".rms", "application/vnd.rn-realmedia"),
    std::make_pair(".rmvb", "application/vnd.rn-realmedia"),
    std::make_pair(".rmx", "application/vnd.rn-realmedia"),
    std::make_pair(".roff", "text/troff"),
    std::make_pair(".rp", "image/vnd.rn-realpix"),
    std::make_pair(".rpm", "application/x-rpm"),
    std::make_pair(".rss", "application/rss+xml"),
    std::make_pair(".rt", "text/vnd.rn-realtext"),
    std::make_pair(".rtf", "application/rtf"),
    std::make_pair(".rtx", "text/richtext"),
    std::make_pair(".rv", "video/vnd.rn-realvideo"),
    std::make_pair(".rvx", "video/vnd.rn-realvideo"),
    std::make_pair(".s3m", "audio/x-s3m"),
    std::make_pair(".sam", "application/x-amipro"),
    std::make_pair(".sami", "application/x-sami"),
    std::make_pair(".sav", "application/x-spss-sav"),
    std::make_pair(".scm", "text/x-scheme"),
    std::make_pair(".sda", "application/vnd.stardivision.draw"),
    std::make_pair(".sdc", "application/vnd.stardivision.calc"),
    std::make_pair(".sdd", "application/vnd.stardivision.impress"),
    std::make_pair(".sdp", "application/sdp"),
    std::make_pair(".sds", "application/vnd.stardivision.chart"),
    std::make_pair(".sdw", "application/vnd.stardivision.writer"),
    std::make_pair(".sgf", "application/x-go-sgf"),
    std::make_pair(".sgi", "image/x-sgi"),
    std::make_pair(".sgl", "application/vnd.stardivision.writer"),
    std::make_pair(".sgm", "text/sgml"),
    std::make_pair(".sgml", "text/sgml"),
    std::make_pair(".sh", "application/x-shellscript"),
    std::make_pair(".shar", "application/x-shar"),
    std::make_pair(".shn", "application/x-shorten"),
    std::make_pair(".siag", "application/x-siag"),
    std::make_pair(".sid", "audio/prs.sid"),
    std::make_pair(".sik", "application/x-trash"),
    std::make_pair(".sis", "application/vnd.symbian.install"),
    std::make_pair(".sisx", "x-epoc/x-sisx-app"),
    std::make_pair(".sit", "application/x-stuffit"),
    std::make_pair(".siv", "application/sieve"),
    std::make_pair(".sk", "image/x-skencil"),
    std::make_pair(".sk1", "image/x-skencil"),
    std::make_pair(".skr", "application/pgp-keys"),
    std::make_pair(".slk", "text/spreadsheet"),
    std::make_pair(".smaf", "application/x-smaf"),
    std::make_pair(".smc", "application/x-snes-rom"),
    std::make_pair(".smd", "application/vnd.stardivision.mail"),
    std::make_pair(".smf", "application/vnd.stardivision.math"),
    std::make_pair(".smi", "application/x-sami"),
    std::make_pair(".smil", "application/smil"),
    std::make_pair(".sml", "application/smil"),
    std::make_pair(".sms", "application/x-sms-rom"),
    std::make_pair(".snd", "audio/basic"),
    std::make_pair(".so", "application/x-sharedlib"),
    std::make_pair(".spc", "application/x-pkcs7-certificates"),
    std::make_pair(".spd", "application/x-font-speedo"),
    std::make_pair(".spec", "text/x-rpm-spec"),
    std::make_pair(".spl", "application/x-shockwave-flash"),
    std::make_pair(".spx", "audio/x-speex"),
    std::make_pair(".sql", "text/x-sql"),
    std::make_pair(".sr2", "image/x-sony-sr2"),
    std::make_pair(".src", "application/x-wais-source"),
    std::make_pair(".srf", "image/x-sony-srf"),
    std::make_pair(".srt", "application/x-subrip"),
    std::make_pair(".ssa", "text/x-ssa"),
    std::make_pair(".stc", "application/vnd.sun.xml.calc.template"),
    std::make_pair(".std", "application/vnd.sun.xml.draw.template"),
    std::make_pair(".sti", "application/vnd.sun.xml.impress.template"),
    std::make_pair(".stm", "audio/x-stm"),
    std::make_pair(".stw", "application/vnd.sun.xml.writer.template"),
    std::make_pair(".sty", "text/x-tex"),
    std::make_pair(".sub", "text/x-subviewer"),
    std::make_pair(".sun", "image/x-sun-raster"),
    std::make_pair(".sv4cpio", "application/x-sv4cpio"),
    std::make_pair(".sv4crc", "application/x-sv4crc"),
    std::make_pair(".svg", "image/svg+xml"),
    std::make_pair(".svgz", "image/svg+xml-compressed"),
    std::make_pair(".swf", "application/x-shockwave-flash"),
    std::make_pair(".sxc", "application/vnd.sun.xml.calc"),
    std::make_pair(".sxd", "application/vnd.sun.xml.draw"),
    std::make_pair(".sxg", "application/vnd.sun.xml.writer.global"),
    std::make_pair(".sxi", "application/vnd.sun.xml.impress"),
    std::make_pair(".sxm", "application/vnd.sun.xml.math"),
    std::make_pair(".sxw", "application/vnd.sun.xml.writer"),
    std::make_pair(".sylk", "text/spreadsheet"),
    std::make_pair(".t", "text/troff"),
    std::make_pair(".t2t", "text/x-txt2tags"),
    std::make_pair(".tar", "application/x-tar"),
    std::make_pair(".tar.bz", "application/x-bzip-compressed-tar"),
    std::make_pair(".tar.bz2", "application/x-bzip-compressed-tar"),
    std::make_pair(".tar.gz", "application/x-compressed-tar"),
    std::make_pair(".tar.lzma", "application/x-lzma-compressed-tar"),
    std::make_pair(".tar.lzo", "application/x-tzo"),
    std::make_pair(".tar.xz", "application/x-xz-compressed-tar"),
    std::make_pair(".tar.z", "application/x-tarz"),
    std::make_pair(".tbz", "application/x-bzip-compressed-tar"),
    std::make_pair(".tbz2", "application/x-bzip-compressed-tar"),
    std::make_pair(".tcl", "text/x-tcl"),
    std::make_pair(".tex", "text/x-tex"),
    std::make_pair(".texi", "text/x-texinfo"),
    std::make_pair(".texinfo", "text/x-texinfo"),
    std::make_pair(".tga", "image/x-tga"),
    std::make_pair(".tgz", "application/x-compressed-tar"),
    std::make_pair(".theme", "application/x-theme"),
    std::make_pair(".themepack", "application/x-windows-themepack"),
    std::make_pair(".tif", "image/tiff"),
    std::make_pair(".tiff", "image/tiff"),
    std::make_pair(".tk", "text/x-tcl"),
    std::make_pair(".tlz", "application/x-lzma-compressed-tar"),
    std::make_pair(".tnef", "application/vnd.ms-tnef"),
    std::make_pair(".tnf", "application/vnd.ms-tnef"),
    std::make_pair(".toc", "application/x-cdrdao-toc"),
    std::make_pair(".torrent", "application/x-bittorrent"),
    std::make_pair(".tpic", "image/x-tga"),
    std::make_pair(".tr", "text/troff"),
    std::make_pair(".ts", "application/x-linguist"),
    std::make_pair(".tsv", "text/tab-separated-values"),
    std::make_pair(".tta", "audio/x-tta"),
    std::make_pair(".ttc", "application/x-font-ttf"),
    std::make_pair(".ttf", "application/x-font-ttf"),
    std::make_pair(".ttx", "application/x-font-ttx"),
    std::make_pair(".txt", "text/plain"),
    std::make_pair(".txz", "application/x-xz-compressed-tar"),
    std::make_pair(".tzo", "application/x-tzo"),
    std::make_pair(".ufraw", "application/x-ufraw"),
    std::make_pair(".ui", "application/x-designer"),
    std::make_pair(".uil", "text/x-uil"),
    std::make_pair(".ult", "audio/x-mod"),
    std::make_pair(".uni", "audio/x-mod"),
    std::make_pair(".uri", "text/x-uri"),
    std::make_pair(".url", "text/x-uri"),
    std::make_pair(".ustar", "application/x-ustar"),
    std::make_pair(".vala", "text/x-vala"),
    std::make_pair(".vapi", "text/x-vala"),
    std::make_pair(".vcf", "text/directory"),
    std::make_pair(".vcs", "text/calendar"),
    std::make_pair(".vct", "text/directory"),
    std::make_pair(".vda", "image/x-tga"),
    std::make_pair(".vhd", "text/x-vhdl"),
    std::make_pair(".vhdl", "text/x-vhdl"),
    std::make_pair(".viv", "video/vivo"),
    std::make_pair(".vivo", "video/vivo"),
    std::make_pair(".vlc", "audio/x-mpegurl"),
    std::make_pair(".vob", "video/mpeg"),
    std::make_pair(".voc", "audio/x-voc"),
    std::make_pair(".vor", "application/vnd.stardivision.writer"),
    std::make_pair(".vst", "image/x-tga"),
    std::make_pair(".wav", "audio/x-wav"),
    std::make_pair(".wax", "audio/x-ms-asx"),
    std::make_pair(".wb1", "application/x-quattropro"),
    std::make_pair(".wb2", "application/x-quattropro"),
    std::make_pair(".wb3", "application/x-quattropro"),
    std::make_pair(".wbmp", "image/vnd.wap.wbmp"),
    std::make_pair(".wcm", "application/vnd.ms-works"),
    std::make_pair(".wdb", "application/vnd.ms-works"),
    std::make_pair(".webm", "video/webm"),
    std::make_pair(".wk1", "application/vnd.lotus-1-2-3"),
    std::make_pair(".wk3", "application/vnd.lotus-1-2-3"),
    std::make_pair(".wk4", "application/vnd.lotus-1-2-3"),
    std::make_pair(".wks", "application/vnd.ms-works"),
    std::make_pair(".wma", "audio/x-ms-wma"),
    std::make_pair(".wmf", "image/x-wmf"),
    std::make_pair(".wml", "text/vnd.wap.wml"),
    std::make_pair(".wmls", "text/vnd.wap.wmlscript"),
    std::make_pair(".wmv", "video/x-ms-wmv"),
    std::make_pair(".wmx", "audio/x-ms-asx"),
    std::make_pair(".wp", "application/vnd.wordperfect"),
    std::make_pair(".wp4", "application/vnd.wordperfect"),
    std::make_pair(".wp5", "application/vnd.wordperfect"),
    std::make_pair(".wp6", "application/vnd.wordperfect"),
    std::make_pair(".wpd", "application/vnd.wordperfect"),
    std::make_pair(".wpg", "application/x-wpg"),
    std::make_pair(".wpl", "application/vnd.ms-wpl"),
    std::make_pair(".wpp", "application/vnd.wordperfect"),
    std::make_pair(".wps", "application/vnd.ms-works"),
    std::make_pair(".wri", "application/x-mswrite"),
    std::make_pair(".wrl", "model/vrml"),
    std::make_pair(".wv", "audio/x-wavpack"),
    std::make_pair(".wvc", "audio/x-wavpack-correction"),
    std::make_pair(".wvp", "audio/x-wavpack"),
    std::make_pair(".wvx", "audio/x-ms-asx"),
    std::make_pair(".x3f", "image/x-sigma-x3f"),
    std::make_pair(".xac", "application/x-gnucash"),
    std::make_pair(".xbel", "application/x-xbel"),
    std::make_pair(".xbl", "application/xml"),
    std::make_pair(".xbm", "image/x-xbitmap"),
    std::make_pair(".xcf", "image/x-xcf"),
    std::make_pair(".xcf.bz2", "image/x-compressed-xcf"),
    std::make_pair(".xcf.gz", "image/x-compressed-xcf"),
    std::make_pair(".xhtml", "application/xhtml+xml"),
    std::make_pair(".xi", "audio/x-xi"),
    std::make_pair(".xla", "application/vnd.ms-excel"),
    std::make_pair(".xlc", "application/vnd.ms-excel"),
    std::make_pair(".xld", "application/vnd.ms-excel"),
    std::make_pair(".xlf", "application/x-xliff"),
    std::make_pair(".xliff", "application/x-xliff"),
    std::make_pair(".xll", "application/vnd.ms-excel"),
    std::make_pair(".xlm", "application/vnd.ms-excel"),
    std::make_pair(".xls", "application/vnd.ms-excel"),
    std::make_pair(".xlsm", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"),
    std::make_pair(".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"),
    std::make_pair(".xlt", "application/vnd.ms-excel"),
    std::make_pair(".xlw", "application/vnd.ms-excel"),
    std::make_pair(".xm", "audio/x-xm"),
    std::make_pair(".xmf", "audio/x-xmf"),
    std::make_pair(".xmi", "text/x-xmi"),
    std::make_pair(".xml", "application/xml"),
    std::make_pair(".xpm", "image/x-xpixmap"),
    std::make_pair(".xps", "application/vnd.ms-xpsdocument"),
    std::make_pair(".xsl", "application/xml"),
    std::make_pair(".xslfo", "text/x-xslfo"),
    std::make_pair(".xslt", "application/xml"),
    std::make_pair(".xspf", "application/xspf+xml"),
    std::make_pair(".xul", "application/vnd.mozilla.xul+xml"),
    std::make_pair(".xwd", "image/x-xwindowdump"),
    std::make_pair(".xyz", "chemical/x-pdb"),
    std::make_pair(".xz", "application/x-xz"),
    std::make_pair(".w2p", "application/w2p"),
    std::make_pair(".z", "application/x-compress"),
    std::make_pair(".zabw", "application/x-abiword"),
    std::make_pair(".zip", "application/zip")
};


class TypeIdentify
{
public:
    static std::string getContentTypeBySuffix(const std::string &suffix)
    {
        LOG(LoggerLevel::INFO, "%s\n", "函数触发");
        if(getTypeIdentifyInstance()->contentType.end() == getTypeIdentifyInstance()->contentType.find(suffix))
        {
            LOG(LoggerLevel::INFO, "未定义后缀类型：%s\n", suffix.c_str());
            // std::cout << "TypeIdentify::getContentTypeBySuffix 未定义后缀类型：" << suffix << std::endl;
            return "";
        }
        LOG(LoggerLevel::INFO, "成功解析后缀文件类型：%s\n", suffix.c_str());
        // std::cout << "TypeIdentify::getContentTypeBySuffix 成功解析后缀文件类型：" << suffix << " (" << contentType[suffix] << ")" << std::endl;
        return contentType[suffix];
    }
    
    static std::string getContentTypeByPath(const std::string &pathName)
    {
        LOG(LoggerLevel::INFO, "%s\n", "函数触发");
        size_t point;
        std::string suffix;
        if ((point = pathName.rfind('.')) != std::string::npos)
        {
            suffix = pathName.substr(point);
        }
        if(suffix.empty())
        {
            LOG(LoggerLevel::ERROR, "解析后缀失败，请求资源路径为：%s（%s）\n", pathName.data(), suffix.c_str());
            // std::cout << "TypeIdentify::getContentTypeByPath 解析后缀失败，请求资源路径为：" << pathName << " (" << suffix << ")" << std::endl;
            return "";
        }
        else
        {
            LOG(LoggerLevel::ERROR, "解析后缀成功，请求资源路径为：%s（%s）\n", pathName.data(), suffix.c_str());
            // std::cout << "TypeIdentify::getContentTypeByPath 成功解析后缀，请求资源路径为：" << pathName << " (" << suffix << ")" << std::endl;
            return getContentTypeBySuffix(suffix);
        }
    }

private:
    TypeIdentify()
    {
        for (auto i : pairArray)
        {
            contentType.insert(std::make_pair(i.first, i.second));
        }
    }
    ~TypeIdentify();
    static TypeIdentify *getTypeIdentifyInstance()
    {
        if(!typeIdentify_)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!typeIdentify_)
            {
                typeIdentify_ = new TypeIdentify();
            }
        }
        return typeIdentify_;
    }

    static std::mutex mutex_;
    static TypeIdentify *typeIdentify_;
    static std::map<std::string, std::string> contentType;
    
};

std::mutex TypeIdentify::mutex_;
TypeIdentify *TypeIdentify::typeIdentify_;
std::map<std::string, std::string> TypeIdentify::contentType;

