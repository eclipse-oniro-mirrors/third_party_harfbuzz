/*
 * Copyright © 2018  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Google Author(s): Behdad Esfahbod
 */

#include "hb.hh"

#include "hb-ot-name-table.hh"

#include "hb-ot-face.hh"
#include "hb-utf.hh"


static inline const OT::name_accelerator_t&
_get_name (hb_face_t *face)
{
  if (unlikely (!hb_ot_shaper_face_data_ensure (face))) return Null(OT::name_accelerator_t);
  return *(hb_ot_face_data (face)->name.get ());
}


unsigned int
hb_ot_name_get_names (hb_face_t                 *face,
		      const hb_ot_name_entry_t **entries /* OUT */)
{
  const OT::name_accelerator_t &name = _get_name (face);
  if (entries)
    *entries = name.names.arrayZ();
  return name.names.len;
}


template <typename in_utf_t, typename out_utf_t>
static inline unsigned int
hb_ot_name_convert_utf (const hb_bytes_t                *bytes,
			unsigned int                    *text_size /* IN/OUT */,
			typename out_utf_t::codepoint_t *text /* OUT */)
{
  unsigned int src_len = bytes->len / sizeof (typename in_utf_t::codepoint_t);
  const typename in_utf_t::codepoint_t *src = (const typename in_utf_t::codepoint_t *) bytes->arrayZ;
  const typename in_utf_t::codepoint_t *src_end = src + src_len;

  typename out_utf_t::codepoint_t *dst = text;

  hb_codepoint_t unicode;
  const hb_codepoint_t replacement = HB_BUFFER_REPLACEMENT_CODEPOINT_DEFAULT;

  if (text_size && *text_size)
  {
    (*text_size)--; /* Same room for NUL-termination. */
    const typename out_utf_t::codepoint_t *dst_end = text + *text_size;

    while (src < src_end && dst < dst_end)
    {
      const typename in_utf_t::codepoint_t *src_next = in_utf_t::next (src, src_end, &unicode, replacement);
      typename out_utf_t::codepoint_t *dst_next = out_utf_t::encode (dst, dst_end, unicode);
      if (dst_next == dst)
        break; /* Out-of-room. */

      dst = dst_next;
      src = src_next;
    };

    *text_size = dst - text;
    *text = 0; /* NUL-terminate. */
  }

  /* Accumulate length of rest. */
  unsigned int dst_len = dst - text;
  while (src < src_end)
  {
    src = in_utf_t::next (src, src_end, &unicode, replacement);
    dst_len += out_utf_t::encode_len (unicode);
  };
  return dst_len;
}

template <typename utf_t>
static inline unsigned int
hb_ot_name_get_utf (hb_face_t     *face,
		    hb_name_id_t   name_id,
		    hb_language_t  language,
		    unsigned int  *text_size /* IN/OUT */,
		    typename utf_t::codepoint_t *text /* OUT */)
{
  const OT::name_accelerator_t &name = _get_name (face);

  unsigned int width;
  int idx = name.get_index (name_id, language, &width);
  if (idx != -1)
  {
    hb_bytes_t bytes = name.table->get_name (idx);

    if (width == 2) /* UTF16-BE */
      return hb_ot_name_convert_utf<hb_utf16_be_t, utf_t> (&bytes, text_size, text);

    if (width == 1) /* ASCII */
      return hb_ot_name_convert_utf<hb_ascii_t, utf_t> (&bytes, text_size, text);
  }

  if (text_size)
  {
    if (*text_size)
      *text = 0;
    *text_size = 0;
  }
  return 0;
}

unsigned int
hb_ot_name_get_utf8 (hb_face_t     *face,
		     hb_name_id_t   name_id,
		     hb_language_t  language,
		     unsigned int  *text_size /* IN/OUT */,
		     char          *text      /* OUT */)
{
  return hb_ot_name_get_utf<hb_utf8_t> (face, name_id, language, text_size,
					(hb_utf8_t::codepoint_t *) text);
}

unsigned int
hb_ot_name_get_utf16 (hb_face_t     *face,
		      hb_name_id_t   name_id,
		      hb_language_t  language,
		      unsigned int  *text_size /* IN/OUT */,
		      uint16_t      *text      /* OUT */)
{
  return hb_ot_name_get_utf<hb_utf16_t> (face, name_id, language, text_size, text);
}

unsigned int
hb_ot_name_get_utf32 (hb_face_t     *face,
		      hb_name_id_t   name_id,
		      hb_language_t  language,
		      unsigned int  *text_size /* IN/OUT */,
		      uint32_t      *text      /* OUT */)
{
  return hb_ot_name_get_utf<hb_utf32_t> (face, name_id, language, text_size, text);
}