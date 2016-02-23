// -*- mode: c++; indent-tabs-mode: nil; -*-
//
// Copyright (c) 2010-2015 Illumina, Inc.
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.

// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/**
 * Count variants
 *
 * \file BlockQuantify.hh
 * \author Peter Krusche
 * \email pkrusche@illumina.com
 *
 */


#include "helpers/StringUtil.hh"
#include "helpers/BCFHelpers.hh"
#include "Variant.hh"
#include "Fasta.hh"
#include "Error.hh"

#include <htslib/hts.h>

#include <array>
#include <list>
#include <htslib/vcf.h>
#include <thread>

#include "BlockQuantify.hh"
#include "BlockQuantifyImpl.hh"

#include "XCmpQuantify.hh"
#include "GA4GHQuantify.hh"

//#define DEBUG_BLOCKQUANTIFY

namespace variant {

    /** factory method */
    std::unique_ptr<BlockQuantify> makeQuantifier(
        bcf_hdr_t * hdr,
        FastaFile const & ref_fasta,
        std::string const & type,
        std::string const & params)
    {
        if(type == "xcmp")
        {
            return std::unique_ptr<BlockQuantify>((BlockQuantify*)new XCMPQuantify(hdr, ref_fasta, params));
        }
        else if(type == "ga4gh")
        {
            return std::unique_ptr<BlockQuantify>((BlockQuantify*)new GA4GHQuantify(hdr, ref_fasta, params));
        }
        else
        {
            error("Unknown quantification method: '%s'", type.c_str());
        }
        return nullptr;
    }

    /** list all quantification methods */
    std::list<std::string> listQuantificationMethods()
    {
        return {"xcmp", "ga4gh"};
    }

    BlockQuantify::BlockQuantifyImpl::~BlockQuantifyImpl()
    {
        for(auto x : variants)
        {
            bcf_destroy(x);
        }
    }

    BlockQuantify::BlockQuantify(bcf_hdr_t * hdr,
                                 FastaFile const & ref_fasta,
                                 std::string const & params) :
        _impl(std::unique_ptr<BlockQuantifyImpl>(new BlockQuantifyImpl{hdr,
                                                    ref_fasta,
                                                    nullptr,
                                                    BlockQuantifyImpl::count_map_t(),
                                                    BlockQuantifyImpl::variantlist_t(),
                                                    bcfhelpers::getSampleNames(hdr),
                                                    params.find("count_unk") != std::string::npos,
                                                    params.find("output_vtc") != std::string::npos,
                                                    params.find("count_homref") != std::string::npos
        }))
    {
    }

    BlockQuantify::~BlockQuantify() {}

    BlockQuantify::BlockQuantify(BlockQuantify && rhs) : _impl(std::move(rhs._impl)) { }

    BlockQuantify & BlockQuantify::operator=(BlockQuantify && rhs)
    {
        _impl = std::move(rhs._impl);
        return *this;
    }

    void BlockQuantify::add(bcf1_t * v)
    {
        _impl->variants.push_back(v);
    }

    void BlockQuantify::count()
    {
        _impl->fasta_to_use.reset(new FastaFile(_impl->ref_fasta));
#ifdef DEBUG_BLOCKQUANTIFY
        int lastpos = 0;
#endif
        for(auto & v : _impl->variants)
        {
#ifdef DEBUG_BLOCKQUANTIFY
            lastpos = v->pos;
#endif
            countVariants(v);
        }
#ifdef DEBUG_BLOCKQUANTIFY
        std::cerr << "finished block " << lastpos << " - " << _impl->variants.size() << " records on thread " << std::this_thread::get_id() << "\n";
#endif
        _impl->fasta_to_use.reset(nullptr);
    }

    // result output
    std::map<std::string, VariantStatistics> const & BlockQuantify::getCounts() const
    {
        return _impl->count_map;
    }

    std::list<bcf1_t*> const & BlockQuantify::getVariants()
    {
        return _impl->variants;
    }

}