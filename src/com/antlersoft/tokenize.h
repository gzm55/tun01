#ifndef COM_ANTLERSOFT_TOKENIZE_H
#define COM_ANTLERSOFT_TOKENIZE_H

template<typename string_type, typename collection_type> void tokenize( collection_type& result, string_type tokens, typename string_type::value_type delimiter)
{
	typename string_type::size_type last_pos=0;
	for ( typename string_type::size_type current_pos=string_type::npos; last_pos<tokens.length()
		&& ( current_pos=tokens.find( delimiter, last_pos))!=string_type::npos; )
	{
		result.push_back( tokens.substr( last_pos, current_pos-last_pos));
		last_pos=current_pos+1;
	}
	if ( last_pos<tokens.length())
	{
		result.push_back( tokens.substr( last_pos));
	}
}

#endif
